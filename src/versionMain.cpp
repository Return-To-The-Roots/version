// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/integration/filesystem.hpp>
#include <boost/nowide/iostream.hpp>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;

void finish()
{
    bnw::cout << "       version: finished" << std::endl;
}

std::string GetCommitFromPackedGitRefs(const std::string& ref, const bfs::path& gitPath)
{
    bfs::path packedRefsPath = gitPath / "packed-refs";
    if(!bfs::is_regular_file(packedRefsPath))
    {
        bnw::cerr << "                failed to find " << packedRefsPath << std::endl;
        return "";
    }
    bnw::ifstream refsFile(packedRefsPath);
    std::string commit, commitRef;
    while(refsFile >> commit)
    {
        // Skip comment
        if(!commit.empty() && commit[0] == '#')
        {
            getline(refsFile, commit);
            continue;
        }
        if(!(refsFile >> commitRef))
            break;
        if(commitRef == ref)
            return commit;
    }
    return "";
}

bool getRevisionOrCommit(const bfs::path& source_dir, unsigned& revision, std::string& commit)
{
    bfs::path bzrPath = source_dir / ".bzr/branch/last-revision";
    bfs::path svnPath = source_dir / ".svn/entries";
    bfs::path gitPath = source_dir / ".git";
    if(!bfs::is_directory(gitPath))
        gitPath = source_dir / "../.git";

    bnw::ifstream bzr(bzrPath), svn(svnPath), githead(gitPath / "HEAD");

    if(!svn && !bzr && !githead)
    {
        bnw::cerr << "                failed to read any of:" << std::endl
                  << "                " << svnPath << std::endl
                  << "                " << bzrPath << std::endl
                  << "                " << (gitPath / "HEAD") << std::endl;

        return false;
    }

    // use git revision if exist
    if(githead)
    {
        std::string ref_text, ref;
        githead >> ref_text >> ref;

        if(ref_text != "ref:")
            commit = ref_text;
        else if(!bfs::is_regular_file(gitPath / ref))
        {
            if(ref.substr(0, 5) != "refs/")
            {
                bnw::cerr << "                failed to read " << gitPath << ". Content: " << ref << std::endl;
                return false;
            }
            commit = GetCommitFromPackedGitRefs(ref, gitPath);
            if(commit.empty())
            {
                bnw::cerr << "                failed to find ref: " << ref << std::endl;
                return false;
            }
        } else
        {
            bnw::ifstream gitref(gitPath / ref);
            if(!gitref)
            {
                bnw::cerr << "                failed to read:" << std::endl;
                bnw::cerr << "                " << (gitPath / ref) << ": " << strerror(errno) << std::endl;
                return false;
            }
            std::getline(gitref, commit);
        }
        if(commit.empty())
        {
            bnw::cerr << "                failed to find git commit: " << std::endl;
            return false;
        }
    } else if(bzr) // use bazaar revision if exist
    {
        if(!(bzr >> revision))
        {
            bnw::cerr << "                failed to find bzr revision: " << std::endl;
            return false;
        }
    } else if(svn) // using subversion revision, if no bazaar one exists
    {
        std::string t;

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

        // "last revision"
        if(!(svn >> revision))
        {
            bnw::cerr << "                failed to find svn revision: " << std::endl;
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[])
{
    bnw::args(argc, argv);
    bnw::nowide_filesystem();

    const std::string versionFileName = "build_version_defines.h";
    const std::string versionFileNameForce = versionFileName + ".force";
    const std::string versionFileNameCMake = versionFileName + ".cmake";

    bfs::path binary_dir = bfs::current_path();

    if(argc >= 2)
    {
        boost::system::error_code ec;
        bfs::current_path(argv[1], ec);
        if(ec)
        {
            bnw::cerr << "chdir to directory \"" << argv[1] << "\" failed!" << std::endl << "Msg: " << ec.message() << std::endl;
            return 1;
        }
    }

    bfs::path source_dir = bfs::current_path();
    bnw::cerr << "       version: started" << std::endl;
    bnw::cerr << "                source directory: " << source_dir << std::endl;
    bnw::cerr << "                build  directory: " << binary_dir << std::endl;

    atexit(finish);

    if(bfs::exists(binary_dir / versionFileNameForce))
    {
        bnw::cerr << "                the file \"" + versionFileNameForce + "\" does exist." << std::endl;
        bnw::cerr << "                I will not change \"" + versionFileName + "\"." << std::endl;
        return 0;
    }

    unsigned revision = 0;
    std::string commit;
    if(!getRevisionOrCommit(source_dir, revision, commit))
        return 1;

    bnw::ifstream versionh(binary_dir / versionFileName);
    std::string openVersionH = versionFileName;
    const int versionh_errno = errno;

    if(!versionh)
    {
        versionh.clear();
        versionh.open(source_dir / versionFileNameCMake);
        openVersionH = versionFileNameCMake;
    }

    if(!versionh)
    {
        bnw::cerr << "                failed to read any of:" << std::endl;
        bnw::cerr << "                " + versionFileName + ":    " << strerror(versionh_errno) << std::endl;
        bnw::cerr << "                " + versionFileNameCMake + ": " << strerror(errno) << std::endl;

        return 1;
    }

    std::vector<std::string> newversionh;

    std::string line;
    bool changed = false;
    while(std::getline(versionh, line))
    {
        std::stringstream sLine(line);
        std::string sDefine;

        sLine >> sDefine;

        if(sDefine == "#define")
        {
            std::string defineName, defineValue;
            char tmpQuote;

            sLine >> defineName >> tmpQuote >> defineValue; // define name "value"
            // Remove the trailing quote
            if(!defineValue.empty() && defineValue[defineValue.size() - 1u] == '"')
                defineValue.resize(defineValue.size() - 1u);

            unsigned defineValInt = 0;
            // TODO: Boost 1.56 has try_lexcial_convert
            try
            {
                defineValInt = boost::lexical_cast<unsigned>(defineValue);
            } catch(const boost::bad_lexical_cast&)
            { //-V565
            }

            if(defineName == "FORCE")
            {
                bnw::cerr << "                the define \"FORCE\" does exist in the file \"" + openVersionH + "\"" << std::endl;
                bnw::cerr << "                I will not change \"" + openVersionH + "\"" << std::endl;
                return 0;
            } else if(defineName == "WINDOW_VERSION")
            {
                time_t t;
                time(&t);

                char tv[64];
                strftime(tv, 63, "%Y%m%d", localtime(&t));
                if(defineValInt >= 20000101u && defineValInt < boost::lexical_cast<unsigned>(tv))
                {
                    // set new day
                    sLine.clear();
                    sLine << sDefine << " " << defineName << " \"" << tv << "\"";
                    line = sLine.str();

                    bnw::cout << "                renewing version to day \"" << tv << "\"" << std::endl;
                    changed = true;
                }
            } else if(defineName == "WINDOW_REVISION")
            {
                if(!commit.empty())
                {
                    if(defineValue != commit)
                    {
                        // set new revision
                        sLine.clear();
                        sLine << sDefine << " " << defineName << " \"" << commit << "\"";
                        line = sLine.str();

                        bnw::cout << "                renewing commit to \"" << commit << "\"" << std::endl;
                        changed = true;
                    }
                } else
                {
                    if(defineValInt < revision)
                    {
                        // set new revision
                        sLine.clear();
                        sLine << sDefine << " " << defineName << " \"" << revision << "\"";
                        line = sLine.str();

                        bnw::cout << "                renewing version to revision \"" << revision << "\"" << std::endl;
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
        bnw::cerr << "                " + openVersionH + " has changed" << std::endl;

        bnw::ofstream versionh(binary_dir / versionFileName);
        const int versionh_errno = errno;

        if(!versionh)
        {
            bnw::cerr << "failed to write to " + versionFileName + ": " << strerror(versionh_errno) << std::endl;
            return 1;
        }

        for(std::vector<std::string>::const_iterator itLine = newversionh.begin(); itLine != newversionh.end(); ++itLine)
            versionh << *itLine << std::endl;

        versionh.close();
    } else
        bnw::cerr << "                " + versionFileName + " is unchanged" << std::endl;

    return 0;
}
