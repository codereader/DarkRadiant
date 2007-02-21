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

#include "Doom3EntityClass.h"

#include "ifilesystem.h"
#include "iarchive.h"

#include "generic/callback.h"
#include "parser/DefTokeniser.h"
#include "os/path.h"
#include "os/dir.h"
#include "moduleobservers.h"

#include <map>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

/* Static map of named entity classes, using case-insensitive comparison */
typedef std::map<std::string, IEntityClassPtr> EntityClasses;
EntityClasses g_EntityClassDoom3_classes;

void EntityClassDoom3_clear()
{
	g_EntityClassDoom3_classes.clear();
}

// entityClass will be inserted only if another of the same name does not already exist.
// if entityClass was inserted, the same object is returned, otherwise the already-existing object is returned.
IEntityClassPtr EntityClassDoom3_insertUnique(IEntityClassPtr entityClass)
{
  return (*g_EntityClassDoom3_classes.insert(EntityClasses::value_type(entityClass->getName(), entityClass)).first).second;
}

class Model
{
public:
  bool m_resolved;
  std::string m_mesh;
  std::string m_skin;
  std::string m_parent;
  typedef std::map<std::string, std::string> Anims;
  Anims m_anims;
  Model() : m_resolved(false)
  {
  }
};

typedef std::map<std::string, Model> Models;

Models g_models;

void Model_resolveInheritance(const char* name, Model& model)
{
  if(model.m_resolved == false)
  {
    model.m_resolved = true;

    if(!string_empty(model.m_parent.c_str()))
    {
      Models::iterator i = g_models.find(model.m_parent);
      if(i == g_models.end())
      {
        globalErrorStream() << "model " << name << " inherits unknown model " << model.m_parent.c_str() << "\n";
      }
      else
      {
        Model_resolveInheritance((*i).first.c_str(), (*i).second);
        model.m_mesh = (*i).second.m_mesh;
        model.m_skin = (*i).second.m_skin;
      }
    }
  }
}

void EntityClassDoom3_parseModel(parser::DefTokeniser& tokeniser)
{
    // Add the named model to the global list
    const std::string name = tokeniser.nextToken(); 
    Model& model = g_models[name];

    tokeniser.assertNextToken("{");

    // State enum
    enum { 
        NONE,   // usual state
        ANIM    // parsed anim, may get a { ... } block with further info
    } state = NONE;

    while (true) {

        const std::string parameter = tokeniser.nextToken();

        if (parameter == "}")
            break;
            
        if (parameter == "inherit") {
            model.m_parent = tokeniser.nextToken().c_str();
        }
        else if (parameter == "mesh") {
            model.m_mesh = tokeniser.nextToken().c_str();
        }
        else if (parameter == "skin") {
            model.m_skin = tokeniser.nextToken().c_str();
        }
        else if (parameter == "offset") {
            tokeniser.skipTokens(5);
        }
        else if (parameter == "channel") {
            // SYNTAX: "channel" <name> "(" <blah> [ <blah> ... ] ")"
            tokeniser.skipTokens(2);
            while (tokeniser.nextToken() != ")");
        }
        else if (parameter == "anim") {
            // SYNTAX: "anim" <name> <md5file> [ "{" <blah> [ <blah> ... ] "}" ]
            std::string name = tokeniser.nextToken();
            std::string file = tokeniser.nextToken();
            model.m_anims.insert(Model::Anims::value_type(name.c_str(), file.c_str()));
            state = ANIM; // check for the braces on the next iteration
        }
        else if (state == ANIM && parameter == "{") { // anim braces
            while (tokeniser.nextToken() != "}");
            state = NONE;
        }
    }
}

void EntityClassDoom3_parseEntityDef(parser::DefTokeniser& tokeniser)
{
    // Get the (lowercase) entity name and create the entity class for it
    const std::string sName = 
    	boost::algorithm::to_lower_copy(tokeniser.nextToken());
	IEntityClassPtr entityClass(new eclass::Doom3EntityClass(sName));

    // Required open brace
    tokeniser.assertNextToken("{");

    // Loop over all of the keys in this entitydef
    while (true) {

        const std::string key = tokeniser.nextToken();
        if (key == "}") // end of def
            break;

        const std::string value = tokeniser.nextToken();
    
        // Otherwise, switch on the key name
        
        if (key == "model") {
        	entityClass->setModelPath(os::standardPath(value));
        }
        else if (key == "inherit") {
        	entityClass->setParent(value);
        }
        else if (key == "editor_color") {
            entityClass->setColour(value);
        }
        else if (key == "editor_mins") {
            if (value != "?") { // what does this mean?
				entityClass->setMins(value);
            }
        }
        else if (key == "editor_maxs") {
            if (value != "?") { // what does this mean?
				entityClass->setMaxs(value);
            }
        }
        else if (key == "editor_light") {
            if (value == "1") {
                entityClass->setIsLight(true);
            }
        }
        else if (key == "spawnclass") {
            if (value == "idLight") {
                entityClass->setIsLight(true);
            }
        }
        else if (boost::algorithm::istarts_with(key, "editor_var ")) {
        	// "editor_var" represents an attribute that may be set on this
        	// entity. Construct a value-less EntityClassAttribute to add to
        	// the class, so that it will show in the entity inspector.
        	std::string attName = key.substr(key.find(" ") + 1);
        	if (!attName.empty()) {
        		entityClass->addAttribute(
        			EntityClassAttribute("text", attName, "", value));
        	}
        }
        else if (boost::algorithm::istarts_with(key, "editor_bool ")) {
			// Same as editor_var, but with boolean rather than text type
        	std::string attName = key.substr(key.find(" ") + 1);
        	if (!attName.empty()) {
        		entityClass->addAttribute(
        			EntityClassAttribute("boolean", attName, "", value));
        	}
        }

		// Following key-specific processing, add the keyvalue to the entity
		// class
		EntityClassAttribute attribute("text", key, value, "");
		entityClass->addAttribute(attribute);
            
    } // while true
    
    // Insert into the EntityClassManager
	EntityClassDoom3_insertUnique(entityClass);
}

// Parse the provided string containing the contents of a single .def file.
// Extract all entitydefs and create objects accordingly.

void EntityClassDoom3_parse(const std::string& inStr)
{
    parser::DefTokeniser tokeniser(inStr);

    while (tokeniser.hasMoreTokens()) {

        std::string blockType = tokeniser.nextToken();
        boost::algorithm::to_lower(blockType);

        if(blockType == "entitydef") {
            EntityClassDoom3_parseEntityDef(tokeniser);
        }
        else if(blockType == "model") {
            EntityClassDoom3_parseModel(tokeniser);
        }
    }

}


void EntityClassDoom3_loadFile(const char* filename)
{
    const std::string fullname = "def/" + std::string(filename);

	ArchiveTextFile* file = GlobalFileSystem().openTextFile(fullname.c_str());
	if(file != 0) {
        try {
            EntityClassDoom3_parse(file->getInputStream().getAsString());
        }
        catch (parser::ParseException e) {
            std::cerr << "[eclassmgr] failed to parse " << filename 
            		  << " (" << e.what() << ")" << std::endl;
        }
        file->release();
	}
}

// Find or insert an EntityClass with the given name.

IEntityClassPtr EntityClassDoom3_findOrInsert(const std::string& name, 
											  bool has_brushes)
{
    // Return an error if no name is given
    if (name.empty()) {
        return IEntityClassPtr();
    }

	// Convert string to lowercase, for case-insensitive lookup
	std::string lName = boost::algorithm::to_lower_copy(name);

    // Find the EntityClass in the map.
    EntityClasses::iterator i = g_EntityClassDoom3_classes.find(lName);
    if (i != g_EntityClassDoom3_classes.end()) {
        return i->second; // found it, return
    }

    // Otherwise insert the new EntityClass
    IEntityClassPtr e = eclass::Doom3EntityClass::create(lName, has_brushes);
    IEntityClassPtr inserted = EntityClassDoom3_insertUnique(e);

    return inserted;
}

//void EntityClass_resolveInheritance(EntityClass* derivedClass)
//{
//  if(derivedClass->inheritanceResolved == false)
//  {
//    derivedClass->inheritanceResolved = true;
//    EntityClasses::iterator i = g_EntityClassDoom3_classes.find(derivedClass->m_parent.front().c_str());
//    if(i == g_EntityClassDoom3_classes.end())
//    {
//      globalErrorStream() << "failed to find entityDef " << makeQuoted(derivedClass->m_parent.front().c_str()) << " inherited by "  << makeQuoted(derivedClass->m_name.c_str()) << "\n";
//    }
//    else
//    {
//      EntityClass* parentClass = (*i).second;
//      EntityClass_resolveInheritance(parentClass);
//      if(!derivedClass->colorSpecified)
//      {
//        derivedClass->colorSpecified = parentClass->colorSpecified;
//        derivedClass->setColour(parentClass->getColour());
//      }
//      if(!derivedClass->sizeSpecified)
//      {
//        derivedClass->sizeSpecified = parentClass->sizeSpecified;
//        derivedClass->mins = parentClass->mins;
//        derivedClass->maxs = parentClass->maxs;
//        derivedClass->fixedsize = parentClass->fixedsize;
//      }
//
//      for(EntityClassAttributes::iterator j = parentClass->m_attributes.begin(); j != parentClass->m_attributes.end(); ++j)
//      {
//        EntityClass_insertAttribute(*derivedClass, (*j).first.c_str(), (*j).second);
//      }
//    }
//  }
//}

/* EntityClassDoom3 - master entity loader
 * 
 * This class is the master loader for the entity classes. It ensures that the
 * EntityClassDoom3_loadFile() function is called for every .def file in the
 * def/ directory, which in turn kicks off the parse process (including the
 * resolution of inheritance.
 * 
 * It also accomodates ModuleObservers, presumably to allow plugins to be
 * notified when entity classes are added or modified.
 */

class EntityClassDoom3:
    public ModuleObserver {

    // Whether the entity classes have been realised
    std::size_t m_unrealised;

    // Set of ModuleObservers to notify on realise/unrealise
    ModuleObservers m_observers;

  public:
    
    // Constructor
    EntityClassDoom3():
        m_unrealised(2) {} 
    
    void realise() {
        
        // Count the number of times this function is called, it is activated
        // for real on the second call (why?)
        if (--m_unrealised == 0) {
            
            globalOutputStream() << "searching vfs directory " <<
                makeQuoted("def") << " for *.def\n";
            GlobalFileSystem().forEachFile("def/", "def",
                                           FreeCaller1 < const char *,
                                           EntityClassDoom3_loadFile > ());
    
            // Resolve inheritance on the model classes
            for (Models::iterator i = g_models.begin(); i != g_models.end(); ++i) {
                Model_resolveInheritance((*i).first.c_str(), (*i).second);
            }
            
            // Resolve inheritance for the entities        
            for (EntityClasses::iterator i = g_EntityClassDoom3_classes.begin();
                 i != g_EntityClassDoom3_classes.end(); ++i) {
    
    			// Tell the entityclass to resolve its inheritance
    			i->second->resolveInheritance();
    
                // If the entity has a model path ("model" key), lookup the actual
                // model and apply its mesh and skin to this entity.
                if (i->second->getModelPath().size() > 0) {
                    Models::iterator j = g_models.find(i->second->getModelPath());
                    if (j != g_models.end()) {
                        i->second->setModelPath(j->second.m_mesh);
                        i->second->setSkin(j->second.m_skin);
                    }
                }
            
            }
    
        // Prod the observers (also on the first call)
        m_observers.realise();
        } // endif
    } // end func

    void unrealise()
    {
        if (++m_unrealised == 1) {
            m_observers.unrealise();
            EntityClassDoom3_clear();
        }
    }

    void attach(ModuleObserver & observer)
    {
        m_observers.attach(observer);
    }
    
    void detach(ModuleObserver & observer)
    {
        m_observers.detach(observer);
    }
};

EntityClassDoom3 g_EntityClassDoom3;

class EntityClassDoom3Dependencies : public GlobalFileSystemModuleRef, public GlobalShaderCacheModuleRef
{
};

/**
 * API wrapper class, implements the IEntityClassManager interface.
 * TODO: Merge this with EntityClassDoom3, rather than dispatching calls to a
 * global object.
 */
class EntityClassDoom3API
: public IEntityClassManager
{
public:
    typedef IEntityClassManager Type;
    STRING_CONSTANT(Name, "doom3");

    // Constructor, attach and realise
    EntityClassDoom3API() {
    	GlobalFileSystem().attach(g_EntityClassDoom3);
		realise();
    }
    
    // Destructor. Destroy the entity class manager.
    ~EntityClassDoom3API() {
		unrealise();
		GlobalFileSystem().detach(g_EntityClassDoom3);
    }
    
    // Return the EntityClassManager object.
    IEntityClassManager* getTable() {
        return this;
    }
    
    /* PUBLIC INTERFACE */
    
    // Get a named entity class, creating if necessary
    virtual IEntityClassPtr findOrInsert(const std::string& name,
    									 bool has_brushes)
	{
		return EntityClassDoom3_findOrInsert(name, has_brushes);
	}
	
	// Visit each entity class
	virtual void forEach(EntityClassVisitor& visitor) {
		for(EntityClasses::iterator i = g_EntityClassDoom3_classes.begin(); 
			i != g_EntityClassDoom3_classes.end(); 
			++i)
		{
			visitor.visit(i->second);
		}
	}
	
	// Attach a module observer
	virtual void attach(ModuleObserver& observer) {
		g_EntityClassDoom3.attach(observer);
	}
	
	// Detach a module observer
	virtual void detach(ModuleObserver& observer) {
		g_EntityClassDoom3.detach(observer);
	}
	
	// Realise the module
	virtual void realise() {
		g_EntityClassDoom3.realise();
	}
	
	// Unrealise the module
	virtual void unrealise() {
		g_EntityClassDoom3.unrealise();
	}
};

/* Required code to register the module with the ModuleServer.
 */

#include "modulesystem/singletonmodule.h"

typedef SingletonModule<EntityClassDoom3API, 
						EntityClassDoom3Dependencies> EntityClassDoom3Module;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
	// Static module instance
	static EntityClassDoom3Module _theEclassModule;
	
	// Initialise and register the module	
	initialiseModule(server);
	_theEclassModule.selfRegister();
}
