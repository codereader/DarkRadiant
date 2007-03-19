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

#if !defined(INCLUDED_SERVER_H)
#define INCLUDED_SERVER_H

#include <string>

class ModuleServer;
ModuleServer& GlobalModuleServer_get();

/** Module loader functor class. This class is used to traverse a directory and
 * load each module into the GlobalModuleServer.
 */
class ModuleLoader
{
	// The path of the directory the loader is searching
	const std::string _path;
	
	// The filename extension which indicates a module (platform-specific)
	const std::string _ext;
	
public:
	// Constructor, pass the path it should search for modules in
	ModuleLoader(const std::string& path);
	
	// File functor, gets called with each file's name in the searched folder
	void operator() (const std::string& fileName) const;
	
	// Static loader algorithm, searches plugins/ and modules/ for .dll/.so files
	static void loadModules(const std::string& root);
};

#endif
