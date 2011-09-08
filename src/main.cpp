// $Id: main.cpp 7521 2011-09-08 20:45:55Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#	include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef __linux__
	#undef _WIN32
#endif

#ifdef _WIN32
#	include <windows.h>
#	define chdir SetCurrentDirectoryA
#else
#	include <unistd.h>
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

std::string getcwd()
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

void finish()
{
	cerr << "       version: finished" << endl;
}

int main(int argc, char *argv[])
{
	std::string binary_dir = getcwd();
	
	if(argc >= 2)
	{
		if(chdir(argv[1]) != 0)
		{
			cerr << "chdir to directory \"" << argv[1] << "\" failed!" << endl;
			return 1;
		}
	}

	std::string source_dir = getcwd();
	cerr << "       version: started" << endl;
	cerr << "                source directory: \"" << source_dir << "\"" << endl;
	cerr << "                build  directory: \"" << binary_dir << "\"" << endl;

	atexit(finish);

	ifstream bzr( (source_dir + ".bzr/branch/last-revision").c_str() );
	const int bzr_errno = errno;

	ifstream svn( (source_dir + ".svn/entries").c_str() );
	const int svn_errno = errno;

	if(!svn && !bzr)
	{
		cerr << "                failed to read any of:" << endl;
		cerr << "                .bzr/branch/last-revision: " << strerror(bzr_errno) << endl;
		cerr << "                .svn/entries: " << strerror(svn_errno) << endl;

		return 1;
	}

	int revision = 0;

	if(bzr) // use bazaar revision if exist
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

		svn >> revision;
		svn.close();
	}
	
	ifstream versionhforce( (binary_dir + "build_version.h.force").c_str() );
	if(versionhforce)
	{
		cerr << "                the file \"build_version.h.force\" does exist."<< endl;
		cerr << "                i will not change \"build_version.h\"." << endl;
		versionhforce.close();
		return 0;
	}

	ifstream versionh( (binary_dir + "build_version.h").c_str() );
	const int versionh_errno = errno;

	if(!versionh)
	{
		versionh.clear();
		versionh.open( (source_dir + "build_version.h.in").c_str() );
	}

	if(!versionh)
	{
		cerr << "                failed to read any of:" << endl;
		cerr << "                build_version.h:    " << strerror(versionh_errno) << endl;
		cerr << "                build_version.h.in: " << strerror(errno) << endl;

		return 1;
	}

	vector<string> newversionh;

	string l;
	bool changed = false;
	while(getline(versionh, l))
	{
		stringstream ll(l);
		string d, n;
		char q;
		int v;

		ll >> d; // define
		ll >> n; // name
		ll >> q; // "
		ll >> v; // value
		ll >> q; // "

		if(n == "FORCE")
		{
	                cerr << "                the define \"FORCE\" does exist in the file \"build_version.h\""<< endl;
                	cerr << "                i will not change \"build_version.h\"" << endl;
		}	

		if(n == "WINDOW_VERSION")
		{
			time_t t;
			time(&t);

			char tv[64];
			strftime(tv, 63, "%Y%m%d", localtime(&t) );
			if(v >= 20000101 && v < atoi(tv))
			{
				// set new day
				ll.clear();
				ll.str("");
				ll << d << " " << n << " \"" << tv << "\"";
				l = ll.str();

				cout << "                renewing version to day \"" << tv << "\"" << endl;
				changed = true;
			}
		}

		if(n == "WINDOW_REVISION")
		{
			if(v < revision)
			{
				// set new revision
				ll.clear();
				ll.str("");
				ll << d << " " << n << " \"" << revision << "\"";
				l = ll.str();

				cout << "                renewing version to revision \"" << revision << "\"" << endl;
				changed = true;
			}
		}

		newversionh.push_back(l);
	}
	versionh.close();

	if(changed) // only write if changed
	{
		std::cerr << "                build_version.h has changed" << std::endl;
		
		ofstream versionh( (binary_dir + "build_version.h").c_str() );
		const int versionh_errno = errno;

		if(!versionh)
		{
			cerr << "failed to write to build_version.h: " << strerror(versionh_errno) << endl;
			return 1;
		}

		for(vector<string>::const_iterator l = newversionh.begin(); l != newversionh.end(); ++l)
			versionh << *l << endl;

		versionh.close();
	}
	else
		std::cerr << "                build_version.h is unchanged" << std::endl;

	return 0;
}
