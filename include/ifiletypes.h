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

#if !defined(INCLUDED_IFILETYPES_H)
#define INCLUDED_IFILETYPES_H

#include <list>
#include "imodule.h"

/**
 * Simple structure to store a file pattern (e.g. "*.map") along with its name
 * (e.g. "Map files").
 */
struct FileTypePattern
{
	// The user-friendly name
	std::string name;
	
	// The mask pattern
	std::string pattern;

	// Constructor with optional initialisation parameters
	FileTypePattern(const std::string& n = "", const std::string& p = "")
    : name(n), pattern(p)
	{ }
};

/**
 * Structure associating a module name with a FileTypePattern.
 */
struct ModuleFileType
{
	// Module name
	std::string moduleName;
	
	// File type pattern
	FileTypePattern filePattern;
	
	// Initialising constructor
	ModuleFileType(const std::string& n, const FileTypePattern& p)
	: moduleName(n), filePattern(p)
	{ }
};

/**
 * List of ModuleFileType objects.
 */
typedef std::list<ModuleFileType> ModuleTypeList;
typedef boost::shared_ptr<ModuleTypeList> ModuleTypeListPtr;

const std::string MODULE_FILETYPES("FileTypes");

/**
 * Interface for the FileType registry module. This module retains a list of
 * FileTypePattern objects along with their associated module names.
 */
class IFileTypeRegistry :
	public RegisterableModule
{
public:
	/**
	 * Add a type to the registry.
	 * 
	 * @param moduleType
	 * The type of the module, e.g. "map".
	 * 
	 * @param moduleName
	 * Name of the module.
	 * 
	 * @param type
	 * The FileTypePattern to associate with this module name.
	 */
	virtual void addType(const std::string& moduleType, 
						 const std::string& moduleName, 
						 const FileTypePattern& type) = 0;
						 
	/**
	 * Get a list of ModuleFileTypes associated with the given module type. If
	 * the moduleType is not found, returns an empty list.
	 * 
	 * @param moduleType
	 * The module category for which a list of types should be retrieved.
	 */
	virtual ModuleTypeListPtr getTypesFor(const std::string& moduleType) = 0;
	
	/**
	 * Find the name of the module which loads the given extension.
	 */
	virtual std::string findModuleName(const std::string& moduleType, const std::string& extension) = 0;
};

inline IFileTypeRegistry& GlobalFiletypes() {
	// Cache the reference locally
	static IFileTypeRegistry& _fileTypes(
		*boost::static_pointer_cast<IFileTypeRegistry>(
			module::GlobalModuleRegistry().getModule(MODULE_FILETYPES)
		)
	);
	return _fileTypes;
}

#endif
