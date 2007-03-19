/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_ENVIRONMENT_H)
#define INCLUDED_ENVIRONMENT_H

#include <string>

/** greebo: A base class initialised right at the startup holding 
 * 			information about the home and application paths. 
 */
class Environment
{
	// The app + home paths
	std::string _appPath;
	std::string _homePath;
	std::string _settingsPath;
	std::string _bitmapsPath;
	
	std::string _mapsPath;

public:
	// Call this with the arguments from main()
	void init(int argc, char* argv[]);
	
	/** greebo: Get the application/home paths
	 */
	std::string getHomePath();
	std::string getAppPath();
	std::string getSettingsPath();
	std::string getBitmapsPath();
	
	// Get/set the path where the .map files are stored
	std::string getMapsPath();
	void setMapsPath(const std::string& path);
	
	// Contains the static instance
	static Environment& Instance();
	
private:
	// Sets up the bitmap path and settings path
	void initPaths();

	// Initialises the arguments
	void initArgs(int argc, char* argv[]);
};

#endif
