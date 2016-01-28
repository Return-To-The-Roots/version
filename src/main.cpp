// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef __linux__
#undef _WIN32
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#   include <windows.h>
#   define chdir !SetCurrentDirectoryA
#	define stat _stat
#else
#   include <unistd.h>
#endif

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

std::string getCurrendWorkDir()
{
    char curdir[4096];
#ifdef _WIN32
    GetCurrentDirectoryA(4096, curdir);
#else
    std::string ignorestupidgccwarning = getcwd(curdir, 4096);
    ignorestupidgccwarning = "";
#endif

    return std::string(curdir) + '/';
}

bool isfile(const std::string& file)
{
    struct stat s;
    if( stat(file.c_str(), &s) == 0 )
        return ((s.st_mode & S_IFREG) == S_IFREG);

    return false;
}

bool isdir(const std::string& dir)
{
    struct stat s;
    if( stat(dir.c_str(), &s) == 0 )
        return ((s.st_mode & S_IFDIR) == S_IFDIR);

    return false;
}

void finish()
{
    cerr << "       version: finished" << endl;
}

int main(int argc, char* argv[])
{
    const std::string versionFileName = "build_version_defines.h";

    std::string binary_dir = getCurrendWorkDir();

    if(argc >= 2)
    {
        if(chdir(argv[1]) != 0)
        {
            cerr << "chdir to directory \"" << argv[1] << "\" failed!" << endl;
            return 1;
        }
    }

    std::string source_dir = getCurrendWorkDir();
    cerr << "       version: started" << endl;
    cerr << "                source directory: \"" << source_dir << "\"" << endl;
    cerr << "                build  directory: \"" << binary_dir << "\"" << endl;

    atexit(finish);

    ifstream bzr( (source_dir + ".bzr/branch/last-revision").c_str() );
    const int bzr_errno = errno;

    ifstream svn( (source_dir + ".svn/entries").c_str() );
    const int svn_errno = errno;

    string gitpath = source_dir + ".git";
    if (isdir(gitpath))
		gitpath += "/";
	else
        gitpath = source_dir + "../.git/";

    ifstream githead( (gitpath + "/HEAD").c_str() );
    const int git_errno = errno;

    int revision = 0;
    std::string commit = "";

    if(!svn && !bzr && !githead)
    {
        cerr << "                failed to read any of:" << endl;
        cerr << "                .svn/entries: " << strerror(svn_errno) << endl;
        cerr << "                .bzr/branch/last-revision: " << strerror(bzr_errno) << endl;
        cerr << "                " << gitpath << "HEAD: " << strerror(git_errno) << endl;

        return 1;
    }

    // use git revision if exist
    if(githead)
    {
        std::string ref_text, ref;
        githead >> ref_text >> ref;
        githead.close();

        if (ref_text != "ref:")
            commit = ref_text;
        else
        {
            if(!isfile( gitpath + ref ))
            {
                cerr << "                failed to read " << gitpath << "HEAD or " << gitpath << ref << endl;
                return 1;
            }

            ifstream gitref( ( gitpath + ref ).c_str() );
            if(!gitref)
            {
                cerr << "                failed to read:" << endl;
                cerr << "                " << gitpath << ref << ": " << strerror(errno) << endl;
                return 1;
            }
            getline(gitref, commit);
            gitref.close();
        }
    }
    else if(bzr) // use bazaar revision if exist
    {
        bzr >> revision;
        bzr.close();
    }
    else if(svn) // using subversion revision, if no bazaar one exists
    {
        string t;

        getline(svn, t); // entry count
        getline(svn, t); // empty
        getline(svn, t); // "dir"
        getline(svn, t); // "revision"
        getline(svn, t); // "wc-url"
        getline(svn, t); // "repo-url"
        getline(svn, t); // empty
        getline(svn, t); // empty
        getline(svn, t); // empty
        getline(svn, t); // "checkin date"

        svn >> revision; // "last revision"
        svn.close();
    }

    ifstream versionhforce( (binary_dir + versionFileName + ".force").c_str() );
    if(versionhforce)
    {
        cerr << "                the file \"" + versionFileName + ".force\" does exist." << endl;
        cerr << "                i will not change \"" + versionFileName + "\"." << endl;
        versionhforce.close();
        return 0;
    }

    ifstream versionh( (binary_dir + versionFileName).c_str() );
    const int versionh_errno = errno;

    if(!versionh)
    {
        versionh.clear();
        versionh.open( (source_dir + versionFileName + ".cmake").c_str() );
    }

    if(!versionh)
    {
        cerr << "                failed to read any of:" << endl;
        cerr << "                " + versionFileName + ":    " << strerror(versionh_errno) << endl;
        cerr << "                " + versionFileName + ".in: " << strerror(errno) << endl;

        return 1;
    }

    vector<string> newversionh;

    string line;
    bool changed = false;
    while(getline(versionh, line))
    {
        stringstream sLine(line);
        string sDefine, defineName;
        char tmpQuote;
        string defineValue;

        sLine >> sDefine;
        
        if(sDefine == "#define")
        {
            sLine >> defineName >> tmpQuote >> defineValue >> tmpQuote; // define name "value"

            stringstream vv(defineValue);

            int defineValInt = 0;
            vv >> defineValInt;

            if(defineName == "FORCE")
            {
                cerr << "                the define \"FORCE\" does exist in the file \"" + versionFileName + "\"" << endl;
                cerr << "                I will not change \"" + versionFileName + "\"" << endl;
                return 0;
            }else if(defineName == "WINDOW_VERSION")
            {
                time_t t;
                time(&t);

                char tv[64];
                strftime(tv, 63, "%Y%m%d", localtime(&t) );
                if(defineValInt >= 20000101 && defineValInt < atoi(tv))
                {
                    // set new day
                    sLine.clear();
                    sLine.str("");
                    sLine << sDefine << " " << defineName << " \"" << tv << "\"";
                    line = sLine.str();

                    cout << "                renewing version to day \"" << tv << "\"" << endl;
                    changed = true;
                }
            }else if(defineName == "WINDOW_REVISION")
            {
                if(!commit.empty())
                {
                    if(defineValue != commit && defineValue != commit+"\"")
                    {
                        // set new revision
                        sLine.clear();
                        sLine.str("");
                        sLine << sDefine << " " << defineName << " \"" << commit << "\"";
                        line = sLine.str();

                        cout << "                renewing commit to \"" << commit << "\"" << endl;
                        changed = true;
                    }
                }
                else
                {
                    if(defineValInt < revision)
                    {
                        // set new revision
                        sLine.clear();
                        sLine.str("");
                        sLine << sDefine << " " << defineName << " \"" << revision << "\"";
                        line = sLine.str();

                        cout << "                renewing version to revision \"" << revision << "\"" << endl;
                        changed = true;
                    }
                }
            }
        }

        newversionh.push_back(line);
    }
    versionh.close();

    if(changed) // only write if changed
    {
        std::cerr << "                " + versionFileName + " has changed" << std::endl;

        ofstream versionh( (binary_dir + versionFileName).c_str() );
        const int versionh_errno = errno;

        if(!versionh)
        {
            cerr << "failed to write to " + versionFileName + ": " << strerror(versionh_errno) << endl;
            return 1;
        }

        for(vector<string>::const_iterator itLine = newversionh.begin(); itLine != newversionh.end(); ++itLine)
            versionh << *itLine << endl;

        versionh.close();
    }
    else
        std::cerr << "                " + versionFileName + " is unchanged" << std::endl;

    return 0;
}
