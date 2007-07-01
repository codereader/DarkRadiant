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

#if !defined(INCLUDED_MODULESYSTEM_MODULESMAP_H)
#define INCLUDED_MODULESYSTEM_MODULESMAP_H

#include "modulesystem.h"
#include "parser/Tokeniser.h"
#include <map>
#include <iostream>

/** greebo: A container for Modules, indexed by name.
 * 
 * 			Use the find() method to retrieve a named module.
 */
template<typename Type>
class ModulesMap : 
	public Modules<Type>
{
	typedef std::map<std::string, Module*> modules_t;
	modules_t _modules;
public:
	~ModulesMap() {
		// Release all the captured modules upon destruction
		for (modules_t::iterator i = _modules.begin(); i != _modules.end(); ++i) {
			i->second->release();
		}
	}

	typedef modules_t::const_iterator iterator;

	iterator begin() const {
		return _modules.begin();
	}
	
	iterator end() const {
		return _modules.end();
	}

	/** greebo: Insert the module into the map. Tries to capture() the module 
	 * 			before insertion and throws an error on failure.
	 */
	void insert(const std::string& name, Module& module) {
		// Try to capture the requested module
		module.capture();
		
		if (!globalModuleServer().getError()) {
			_modules.insert(modules_t::value_type(name, &module));
		}
		else {
			// Capture failed, throw an error.
			std::cerr << "[modulesystem] Warning: ModulesMap<" 
    				  << typename Type::Name() << "> failed to instantiate module "
    				  << "\"" << name << "\"" << std::endl;
    				  
    		// Release the module again
			module.release();
			globalModuleServer().setError(false);
		} 
	}

	Type* find(const std::string& name) {
		// Try to lookup the named module
		modules_t::iterator i = _modules.find(name);
		
		if (i != _modules.end()) {
			return static_cast<Type*>(Module_getTable(*i->second));
		}
		// No module found
		return NULL;
	}

	// A tautology to the above method find()
	Type* findModule(const std::string& name) {
		return find(name);
	}
	
	/** greebo: Traverse the contained modules with the given Module::Visitor
	 */
	void foreachModule(typename Modules<Type>::Visitor& visitor) {
		for (modules_t::iterator i = _modules.begin(); i != _modules.end(); ++i) {
			visitor.visit(
				i->first.c_str(), 
				*static_cast<const Type*>(Module_getTable(*i->second))
			);
		}
	}
};

/** greebo: Visitor class, inserting all visited modules into the given map.
 */
template<typename Type>
class InsertModules : 
	public ModuleServer::Visitor
{
	ModulesMap<Type>& _modules;
public:
	InsertModules(ModulesMap<Type>& modules) : 
		_modules(modules)
	{}
	
	void visit(const char* name, Module& module) {
		_modules.insert(name, module);
	}
};

// The ModulesRef class appears to be a container for a certain subset of Modules specified in
// its constructor
template<typename Type>
class ModulesRef
{
	ModulesMap<Type> _modules;
public:

	ModulesRef(const std::string& names) {
		// Don't proceed, if the server is already in the "error" state
		if (!globalModuleServer().getError()) {
			// Check if the argument is "*" => load all modules into the map
			if (names == "*") {
				// Instantiate a list populator and load all the relevant modules
				InsertModules<Type> visitor(_modules);
				globalModuleServer().foreachModule(typename Type::Name(), typename Type::Version(), visitor);
			}
			else if (!names.empty()) {
				// Setup a tokeniser to decompose the argument string
				parser::StringTokeniser tokeniser(names);
				
				while (tokeniser.hasMoreTokens()) {
					// Retrieve the next token = the next name
					std::string name = tokeniser.nextToken();
					
					// Try to find the module with this token
					Module* module = globalModuleServer().findModule(
						typename Type::Name(), 
						typename Type::Version(), 
						name.c_str()
					);
					
					if (module != NULL) {
						// Module was found 
						_modules.insert(name, *module);
					}
					else {
						// Module not found in the global module server
						globalModuleServer().setError(true);
						//globalErrorStream() << "ModulesRef::initialise: type=" << makeQuoted(typename Type::Name()) << " version=" << makeQuoted(typename Type::Version()) << " name=" << makeQuoted(name) << " - not found\n";
						break;
					}
	          	}
			}
			else {
				// Empty names argument, don't continue
				throw std::runtime_error("ModulesRef: empty names argument encountered.");
			}
		}
	}
	
	// Return the internal ModulesMap
	ModulesMap<Type>& get() {
		return _modules;
	}
};

#endif
