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

#include "eclass_doom3.h"

#include "debugging/debugging.h"

#include <map>

#include "ifilesystem.h"
#include "iscriplib.h"
#include "iarchive.h"
#include "qerplugin.h"

#include "generic/callback.h"
#include "string/string.h"
#include "parser/DefTokeniser.h"
#include "eclasslib.h"
#include "os/path.h"
#include "os/dir.h"
#include "stream/stringstream.h"
#include "moduleobservers.h"
#include "stringio.h"

#include "gtkutil/dialog.h"

#include <iostream>

#include <boost/algorithm/string/case_conv.hpp>

class RawString
{
  const char* m_value;
public:
  RawString(const char* value) : m_value(value)
  {
  }
  const char* c_str() const
  {
    return m_value;
  }
};

inline bool operator<(const RawString& self, const RawString& other)
{
  return string_less_nocase(self.c_str(), other.c_str());
}

typedef std::map<RawString, EntityClass*> EntityClasses;
EntityClasses g_EntityClassDoom3_classes;
EntityClass	*g_EntityClassDoom3_bad = 0;


void EntityClassDoom3_clear()
{
  for(EntityClasses::iterator i = g_EntityClassDoom3_classes.begin(); i != g_EntityClassDoom3_classes.end(); ++i)
  {
    (*i).second->free((*i).second);
  }
  g_EntityClassDoom3_classes.clear();
}

// entityClass will be inserted only if another of the same name does not already exist.
// if entityClass was inserted, the same object is returned, otherwise the already-existing object is returned.
EntityClass* EntityClassDoom3_insertUnique(EntityClass* entityClass)
{
  return (*g_EntityClassDoom3_classes.insert(EntityClasses::value_type(entityClass->name(), entityClass)).first).second;
}

void EntityClassDoom3_forEach(EntityClassVisitor& visitor)
{
  for(EntityClasses::iterator i = g_EntityClassDoom3_classes.begin(); i != g_EntityClassDoom3_classes.end(); ++i)
  {
    visitor.visit((*i).second);
  }
}

void EntityClassDoom3_parseUnknown(Tokeniser& tokeniser)
{
  //const char* name = 
  tokeniser.getToken();

  //globalOutputStream() << "parsing unknown block " << makeQuoted(name) << "\n";

  const char* token = tokeniser.getToken();
  ASSERT_MESSAGE(string_equal(token, "{"), "error parsing entity definition");
  tokeniser.nextLine();

  std::size_t depth = 1;
  for(;;)
  {
    const char* token = tokeniser.getToken();
    if(string_equal(token, "}"))
    {
      if(--depth == 0)
      {
        tokeniser.nextLine();
        break;
      }
    }
    else if(string_equal(token, "{"))
    {
      ++depth;
    }
    tokeniser.nextLine();
  }
}


class Model
{
public:
  bool m_resolved;
  CopiedString m_mesh;
  CopiedString m_skin;
  CopiedString m_parent;
  typedef std::map<CopiedString, CopiedString> Anims;
  Anims m_anims;
  Model() : m_resolved(false)
  {
  }
};

typedef std::map<CopiedString, Model> Models;

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
    std::cout << "  model name = " << name << std::endl;
    Model& model = g_models[name.c_str()];

    tokeniser.assertNextToken("{");

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
    }

//    else if(string_equal(parameter, "anim"))
//    {
//      CopiedString animName(tokeniser.getToken());
//      const char* animFile = tokeniser.getToken();
//      model.m_anims.insert(Model::Anims::value_type(animName, animFile));
//
//      const char* token = tokeniser.getToken();
//
//      while(string_equal(token, ","))
//      {
//        animFile = tokeniser.getToken();
//        token = tokeniser.getToken();
//      }
//
//      if(string_equal(token, "{"))
//      {
//        for(;;)
//        {
//          const char* end = tokeniser.getToken();
//          if(string_equal(end, "}"))
//          {
//            tokeniser.nextLine();
//            break;
//          }
//          tokeniser.nextLine();
//        }
//      } 
//      else
//      {
//        tokeniser.ungetToken();
//      }
//    }
//    else
//    {
//      ERROR_MESSAGE("unknown model parameter: " << makeQuoted(parameter));
//    }
//    tokeniser.nextLine();
//  }
}

void EntityClassDoom3_parseEntityDef(parser::DefTokeniser& tokeniser)
{
    EntityClass* entityClass = Eclass_Alloc();
    entityClass->free = &Eclass_Free;

    // Entity name
    const std::string sName = tokeniser.nextToken();
    std::cout << "  entitydef name = " << sName << std::endl;
    entityClass->m_name = sName.c_str();

    // Required open brace
    tokeniser.assertNextToken("{");

    // Loop over all of the keys in this entitydef
    while (true) {

        const std::string key = tokeniser.nextToken();

        // DEBUG
//        std::cout << "key is " << key << std::endl;

        if (key == "}") // end of def
            break;
    
        // Otherwise, switch on the key name
        
        if (key == "model") {
            entityClass->fixedsize = true;
            entityClass->m_modelpath = os::standardPath(tokeniser.nextToken()).c_str();
        }
        else if (key == "editor_color") {
            entityClass->colorSpecified = true;
            entityClass->color = Vector3(tokeniser.nextToken());
        }
        else if (key == "editor_mins") {
            entityClass->sizeSpecified = true;
            const std::string value = tokeniser.nextToken();
            if (value != "?") { // what does this mean?
                entityClass->fixedsize = true;
                entityClass->mins = Vector3(value);
            }
        }
        else if (key == "editor_maxs") {
            entityClass->sizeSpecified = true;
            const std::string value = tokeniser.nextToken();
            if (value != "?") { // what does this mean?
                entityClass->fixedsize = true;
                entityClass->maxs = Vector3(value);
            }
        }
        else if (key == "editor_usage") {
            entityClass->m_comments = tokeniser.nextToken().c_str();
        }
        else if (key == "editor_light") {
            const std::string val = tokeniser.nextToken();
            if (val == "1") {
                entityClass->isLight(true);
            }
            else {
                entityClass->isLight(false);
            }
        }
        else if (key.find("editor_") == 0) { // ignore any other "editor_xxx" key
            tokeniser.nextToken();
        }
        else { // any other key is added as an EntityClassAttribute
            EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, key.c_str()).second;
            attribute.m_type = "string";
            attribute.m_value = tokeniser.nextToken().c_str();
        }
            
    } // while true
    
//    else if(!string_empty(last) && (string_equal(first.c_str(), "editor_var") || string_equal(first.c_str(), "editor_string")))
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, last).second;
//      attribute.m_type = "string";
//      currentDescription = &attribute.m_description;
//      currentString = &description;
//      description << tokeniser.getToken();
//    }
//    else if(!string_empty(last) && string_equal(first.c_str(), "editor_float"))
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, last).second;
//      attribute.m_type = "string";
//      currentDescription = &attribute.m_description;
//      currentString = &description;
//      description << tokeniser.getToken();
//    }
//    else if(!string_empty(last) && string_equal(first.c_str(), "editor_snd"))
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, last).second;
//      attribute.m_type = "sound";
//      currentDescription = &attribute.m_description;
//      currentString = &description;
//      description << tokeniser.getToken();
//    }
//    else if(!string_empty(last) && string_equal(first.c_str(), "editor_bool"))
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, last).second;
//      attribute.m_type = "boolean";
//      currentDescription = &attribute.m_description;
//      currentString = &description;
//      description << tokeniser.getToken();
//    }
//    else if(!string_empty(last) && string_equal(first.c_str(), "editor_int"))
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, last).second;
//      attribute.m_type = "integer";
//      currentDescription = &attribute.m_description;
//      currentString = &description;
//      description << tokeniser.getToken();
//    }
//    else if(!string_empty(last) && string_equal(first.c_str(), "editor_model"))
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, last).second;
//      attribute.m_type = "model";
//      currentDescription = &attribute.m_description;
//      currentString = &description;
//      description << tokeniser.getToken();
//    }
//    else if(!string_empty(last) && string_equal(first.c_str(), "editor_color"))
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, last).second;
//      attribute.m_type = "color";
//      currentDescription = &attribute.m_description;
//      currentString = &description;
//      description << tokeniser.getToken();
//    }
//    else if(!string_empty(last) && (string_equal(first.c_str(), "editor_material") || string_equal(first.c_str(), "editor_mat")))
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, last).second;
//      attribute.m_type = "shader";
//      currentDescription = &attribute.m_description;
//      currentString = &description;
//      description << tokeniser.getToken();
//    }
//    else if(string_equal(key, "inherit"))
//    {
//      entityClass->inheritanceResolved = false;
//      ASSERT_MESSAGE(entityClass->m_parent.empty(), "only one 'inherit' supported per entityDef");
//      entityClass->m_parent.push_back(tokeniser.getToken());
//    }
//    else
//    {
//      ASSERT_MESSAGE(!string_equal_n(key, "editor_", 7), "unsupported editor key: " << makeQuoted(key));
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, key).second;
//      attribute.m_type = "string";
//      attribute.m_value = tokeniser.getToken();
//    }
//    tokeniser.nextLine();
//  }
//
//  entityClass->m_comments = usage.c_str();
//
//  if(string_equal(entityClass->m_name.c_str(), "light"))
//  {
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, "light_radius").second;
//      attribute.m_type = "vector3";
//      attribute.m_value = "320 320 320";
//    }
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, "light_center").second;
//      attribute.m_type = "vector3";
//    }
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, "noshadows").second;
//      attribute.m_type = "boolean";
//      attribute.m_value = "0";
//    }
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, "nospecular").second;
//      attribute.m_type = "boolean";
//      attribute.m_value = "0";
//    }
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, "nodiffuse").second;
//      attribute.m_type = "boolean";
//      attribute.m_value = "0";
//    }
//    {
//      EntityClassAttribute& attribute = EntityClass_insertAttribute(*entityClass, "falloff").second;
//      attribute.m_type = "real";
//    }
//  }

  EntityClass* inserted = EntityClassDoom3_insertUnique(entityClass);
  if(inserted != entityClass)
  {
    globalErrorStream() << "entityDef " << entityClass->name() << " is already defined, second definition ignored\n";
    eclass_capture_state(entityClass); // finish constructing the entity so that it can be destroyed cleanly.
    entityClass->free(entityClass);
  }
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
            std::cout << "ENTITYDEF" << std::endl;
            EntityClassDoom3_parseEntityDef(tokeniser);
        }
        else if(blockType == "model") {
            std::cout << "MODEL" << std::endl;
            EntityClassDoom3_parseModel(tokeniser);
        }
//        else {
//            EntityClassDoom3_parseUnknown(tokeniser);
//        }
    }

}


void EntityClassDoom3_loadFile(const char* filename)
{
  globalOutputStream() << "parsing entity classes from " << makeQuoted(filename) << "\n";

    const std::string fullname = "def/" + std::string(filename);

//    std::cout << "** FILE: " << fullname << " **" << std::endl;

	ArchiveTextFile* file = GlobalFileSystem().openTextFile(fullname.c_str());
  if(file != 0)
  {
        try {
            EntityClassDoom3_parse(file->getString());
        }
        catch (parser::ParseException e) {
            gtkutil::errorDialog("Unable to parse DEF file: " + std::string(filename) + "\n\n" + e.what());
        }
        file->release();
  }
}

// Find or insert an EntityClass with the given name.

EntityClass *EntityClassDoom3_findOrInsert(const char *name, bool has_brushes)
{
    ASSERT_NOTNULL(name);

    // Return an error if no name is given
    if (string_empty(name)) {
        return g_EntityClassDoom3_bad;
    }

    // Find the EntityClass in the map.
    EntityClasses::iterator i = g_EntityClassDoom3_classes.find(name);
    if (i != g_EntityClassDoom3_classes.end()) {
        return i->second; // found it, return
    }

    // Otherwise insert the new EntityClass
    EntityClass *e = EntityClass_Create_Default(name, has_brushes);
    EntityClass *inserted = EntityClassDoom3_insertUnique(e);

    ASSERT_MESSAGE(inserted == e, "");
    return inserted;
}

const ListAttributeType* EntityClassDoom3_findListType(const char* name)
{
  return 0;
}


void EntityClass_resolveInheritance(EntityClass* derivedClass)
{
  if(derivedClass->inheritanceResolved == false)
  {
    derivedClass->inheritanceResolved = true;
    EntityClasses::iterator i = g_EntityClassDoom3_classes.find(derivedClass->m_parent.front().c_str());
    if(i == g_EntityClassDoom3_classes.end())
    {
      globalErrorStream() << "failed to find entityDef " << makeQuoted(derivedClass->m_parent.front().c_str()) << " inherited by "  << makeQuoted(derivedClass->m_name.c_str()) << "\n";
    }
    else
    {
      EntityClass* parentClass = (*i).second;
      EntityClass_resolveInheritance(parentClass);
      if(!derivedClass->colorSpecified)
      {
        derivedClass->colorSpecified = parentClass->colorSpecified;
        derivedClass->color = parentClass->color;
      }
      if(!derivedClass->sizeSpecified)
      {
        derivedClass->sizeSpecified = parentClass->sizeSpecified;
        derivedClass->mins = parentClass->mins;
        derivedClass->maxs = parentClass->maxs;
        derivedClass->fixedsize = parentClass->fixedsize;
      }

      for(EntityClassAttributes::iterator j = parentClass->m_attributes.begin(); j != parentClass->m_attributes.end(); ++j)
      {
        EntityClass_insertAttribute(*derivedClass, (*j).first.c_str(), (*j).second);
      }
    }
  }
}

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
    
                EntityClass_resolveInheritance(i->second);
    
                // If the entity has a model path ("model" key), lookup the actual
                // model and apply its mesh and skin to this entity.
                if (!string_empty(i->second->m_modelpath.c_str())) {
                    Models::iterator j = g_models.find(i->second->m_modelpath);
                    if (j != g_models.end()) {
                        i->second->m_modelpath = j->second.m_mesh;
                        i->second->m_skin = j->second.m_skin;
                    }
                }
                eclass_capture_state((*i).second);
            
                StringOutputStream usage(256);
            
                usage << "-------- KEYS --------\n";
            
                // For each attribute in the entity, add its name and description
                // to the usage string, which is stored in the EntityClass' comments
                // field.
                for (EntityClassAttributes::iterator j = i->second->m_attributes.begin();
                     j != i->second->m_attributes.end(); ++j) {
                        
                    const char *name = EntityClassAttributePair_getName(*j);
                    const char *description = EntityClassAttributePair_getDescription(*j);
                
                    if (!string_equal(name, description)) {
                        usage << EntityClassAttributePair_getName(*j)
                            << " : " << EntityClassAttributePair_getDescription(*j)
                            << "\n";
                    }
                }
            
                i->second->m_comments = usage.c_str();
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

void EntityClassDoom3_attach(ModuleObserver& observer)
{
  g_EntityClassDoom3.attach(observer);
}
void EntityClassDoom3_detach(ModuleObserver& observer)
{
  g_EntityClassDoom3.detach(observer);
}

void EntityClassDoom3_realise()
{
  g_EntityClassDoom3.realise();
}
void EntityClassDoom3_unrealise()
{
  g_EntityClassDoom3.unrealise();
}

// Construct the EntityClassDoom3 manager.

void EntityClassDoom3_construct()
{
    std::cout << "EntityClassDoom3_construct()" << std::endl;
    GlobalFileSystem().attach(g_EntityClassDoom3);

    // start by creating the default unknown eclass
    g_EntityClassDoom3_bad = EClass_Create("UNKNOWN_CLASS", Vector3(0.0f, 0.5f, 0.0f), "");

    EntityClassDoom3_realise();
}

void EntityClassDoom3_destroy()
{
  EntityClassDoom3_unrealise();

  g_EntityClassDoom3_bad->free(g_EntityClassDoom3_bad);

  GlobalFileSystem().detach(g_EntityClassDoom3);
}

class EntityClassDoom3Dependencies : public GlobalFileSystemModuleRef, public GlobalShaderCacheModuleRef
{
};

/* EntityClassDoom3API
 * 
 * This class contains the EntityClassManager object for Doom 3, which is
 * returned through calls to GlobalEntityClassManager()::getTable() as per
 * the plugin API. The EntityClassManager member object contains methods which
 * point to C functions in this file.
 */

class EntityClassDoom3API
{
    // EntityClassManager member object
    EntityClassManager m_eclassmanager;

public:
    typedef EntityClassManager Type;
    STRING_CONSTANT(Name, "doom3");

    // Constructor. Create the entity class manager, then set the API function
    // pointers to the appropriate functions.
    EntityClassDoom3API() {
        EntityClassDoom3_construct();
    
        m_eclassmanager.findOrInsert = &EntityClassDoom3_findOrInsert;
        m_eclassmanager.findListType = &EntityClassDoom3_findListType;
        m_eclassmanager.forEach = &EntityClassDoom3_forEach;
        m_eclassmanager.attach = &EntityClassDoom3_attach;
        m_eclassmanager.detach = &EntityClassDoom3_detach;
        m_eclassmanager.realise = &EntityClassDoom3_realise;
        m_eclassmanager.unrealise = &EntityClassDoom3_unrealise;
    }
    
    // Destructor. Destroy the entity class manager.
    ~EntityClassDoom3API() {
        EntityClassDoom3_destroy();
    }
    
    // Return the EntityClassManager object.
    EntityClassManager *getTable()
    {
        return &m_eclassmanager;
    }
};

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

typedef SingletonModule<EntityClassDoom3API, EntityClassDoom3Dependencies> EntityClassDoom3Module;
typedef Static<EntityClassDoom3Module> StaticEntityClassDoom3Module;
StaticRegisterModule staticRegisterEntityClassDoom3(StaticEntityClassDoom3Module::instance());
