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

#include "filetypes.h"

#include "ifiletypes.h"
#include "os/path.h"
#include "stream/textstream.h"

#include <map>
#include <iostream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "modulesystem/StaticModule.h"

/**
 * Implementation of the file type registry.
 */
class RadiantFileTypeRegistry 
: public IFileTypeRegistry
{
	// Map of named ModuleTypeListPtrs. Each ModuleTypeList is a list of structs
	// associating a module name with a filetype pattern
	typedef std::map<std::string, ModuleTypeListPtr> TypeListMap;
	TypeListMap _typeLists;

public:

	/*
	 * Constructor, adds the All Files type.
	 */
	RadiantFileTypeRegistry() {
		addType("*", "*", FileTypePattern("All Files", "*.*"));
	}
	
	/*
	 * Add a type.
	 */
	void addType(const std::string& moduleType, 
				 const std::string& moduleName, 
				 const FileTypePattern& type)
	{
		// Create the association between module name and file type pattern
		ModuleFileType fileType(moduleName, type);
		
		// If there is already a list for this type, add our new type to the
		// back of it
		TypeListMap::iterator i = _typeLists.find(moduleType);
		if (i != _typeLists.end()) {
			i->second->push_back(fileType);
		}
		else {
			// Otherwise create a new type list and add it to our map
			ModuleTypeListPtr newList(new ModuleTypeList());
			newList->push_back(fileType);
			
			_typeLists.insert(TypeListMap::value_type(moduleType, newList));
		}
	}
  
	/*
	 * Return list of types for an associated module type.
	 */
	ModuleTypeListPtr getTypesFor(const std::string& moduleType) {
		
		// Try to find the type list in the map
		TypeListMap::iterator i = _typeLists.find(moduleType);
		if (i != _typeLists.end()) {
			return i->second;
		}
		else {
			// Create a pointer to an empty ModuleTypeList and return this
			// instead of a null shared_ptr
			return ModuleTypeListPtr(new ModuleTypeList());
		}		
	}
	
	// Look for a module which loads the given extension, by searching under the
	// given type category
	virtual std::string findModuleName(const std::string& moduleType, const std::string& extension) {
		// Convert the file extension to lowercase
		std::string ext = boost::algorithm::to_lower_copy(extension);
		
		// Get the list of types for the type category
		ModuleTypeListPtr list = GlobalFiletypes().getTypesFor(moduleType);
		
		// Search in the list for the given extension
		for (ModuleTypeList::const_iterator i = list->begin();
			 i != list->end();
			 i++)
		{
			std::string patternExt = os::getExtension(i->filePattern.pattern);
			if (patternExt == ext) {
				// Match
				return i->moduleName;	
			}
		}
		
		// Not found, return empty string
		return "";
	}
	
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_FILETYPES);
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "FileTypeRegistry::initialiseModule called.\n";
	}
};

// Define the static FileType module
module::StaticModule<RadiantFileTypeRegistry> fileTypeRegistryModule;
