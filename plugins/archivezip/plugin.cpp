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

#include "plugin.h"

#include "imodule.h"
#include "iarchive.h"
#include "stream/textstream.h"

#include <iostream>

#include "ZipArchive.h"

class ArchivePK4API :
	public ArchiveLoader
{
public:
	// greebo: Returns the opened file or NULL if failed.
	virtual ArchivePtr openArchive(const std::string& name) {
		return ZipArchivePtr(new ZipArchive(name));
	}
		
	virtual const std::string& getExtension() {
		static std::string _ext("PK4");
		return _ext;
	}
  
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ArchivePK4");
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "ArchivePK4::initialiseModule called\n";
	}
};
typedef boost::shared_ptr<ArchivePK4API> ArchivePK4APIPtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(ArchivePK4APIPtr(new ArchivePK4API));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
