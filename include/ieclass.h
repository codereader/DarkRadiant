/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

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

/// \file ieclass.h
/// \brief Entity Class definition loader API.


#if !defined(INCLUDED_IECLASS_H)
#define INCLUDED_IECLASS_H

#include "generic/constant.h"
#include "math/Vector3.h"
#include "modulesystem.h"

class Shader;
class ListAttributeType;

/** Data structure representing a single attribute on an entity class.
 */

struct EntityClassAttribute
{
	// The key type (string, bool etc.)
	std::string type;
	
	// The attribute key name
	std::string name;
	
	// Current attribute value
	std::string value;
	
	// The help text associated with the key (in the DEF file)
	std::string description;

	// Default constructor
	EntityClassAttribute() {}

	// Main constructor
	EntityClassAttribute(const std::string& t, 
						 const std::string& n, 
						 const std::string& v = "", 
						 const std::string& d = "") 
	: type(t), name(n), value(v), description(d)
	{}
};


/** Entity class interface. An entity class represents a single type
 * of entity that can be created by the EntityCreator.
 */
struct IEntityClass {

	/** Get this entity class' name.
	 */
	virtual const std::string& getName() const = 0;

	/** Query whether this entity has a fixed size.
	 */
	virtual bool isFixedSize() const = 0;

	/** Set the minimum display size.
	 */
	virtual void setMins(const Vector3& mins) = 0;
	
	/** Set the maximum display size.
	 */
	virtual void setMaxs(const Vector3& maxs) = 0;
	
	/** Get the minimum display size.
	 */
	virtual Vector3 getMins() const = 0;
	
	/** Get the maximum display size.
	 */
	virtual Vector3 getMaxs() const = 0;
	
	/** Set the usage information for this entity class.
	 */
	virtual void setUsage(const std::string&) = 0;

	/** Query whether this entity class represents a light.
	 */
	virtual bool isLight() const = 0;
	
	/** Set whether this entity class represents a light.
	 */
	virtual void setIsLight(bool isLight) = 0;
		
	/** Set this entity class' display colour.
	 * 
	 * @param col
	 * Vector3 containing the R,G,B values to use.
	 */
	virtual void setColour(const Vector3& col) = 0;
	
	/** Return this entity class' display colour.
	 * 
	 * @returns
	 * Vector3 reference containing the colour.
	 */
	virtual const Vector3& getColour() const = 0;
	
	/** Get the Shader used for rendering this entity class in
	 * wireframe mode.
	 */
	virtual Shader* getWireShader() const = 0;
	
	/** Get the Shader used for rendering this entity class in
	 * filled mode.
	 */
	virtual Shader* getFillShader() const = 0;
		
	/** Insert an EntityClassAttribute.
	 */
	virtual void addAttribute(const EntityClassAttribute& attribute) = 0;

	/** Return the value associated with a given entity class attribute.
	 * 
	 * @param key
	 * The key to lookup
	 * 
	 * @returns
	 * The string value of the key, or an empty string if the key is not
	 * found.
	 */
	virtual std::string getValueForKey(const std::string& key) const = 0;

	/** Set a model path on this entity.
	 * 
	 * @param model
	 * The model path to use.
	 */
	virtual void setModelPath(const std::string& model) = 0;
	
	/** Retrieve the model path for this entity.
	 * 
	 * @returns
	 * The VFS model path, or the empty string if there is no model.
	 */
	virtual const std::string& getModelPath() const = 0;
	
	/** Set the model skin.
	 */
	virtual void setSkin(const std::string&) = 0;
	
	/** Get the model skin, or the empty string if there is no skin.
	 */
	virtual const std::string& getSkin() const = 0;
	
	/** Set the parent class. This is specified via an "inherit" key in the
	 * definition.
	 * 
	 * @param parent
	 * The parent entityclass name.
	 */
	virtual void setParent(const std::string& parent) = 0;
	
	/** Return the parent classname, or the empty string if there is no parent.
	 */
	virtual const std::string& getParent() const = 0;
	
	/** Set the parent IEntityClass object. This is used for inheritance
	 * resolution.
	 */
	virtual void setParentEntity(IEntityClass*) = 0;
	
	/** Trigger a recursive inheritance resolution. The parent IEntityClass
	 * will be asked to resolve its own inheritance, then all relevant properties
	 * will be copied onto this class.
	 * 
	 * Resolution stops when an IEntityClass has no parent.
	 */
	virtual void resolveInheritance() = 0;
};


/** EntityClass visitor interface.
 */

class EntityClassVisitor
{
public:
	virtual void visit(IEntityClass* eclass) = 0;
};

class ModuleObserver;


/** EntityClassManager interface. The entity class manager 
 * is responsible for maintaining a list of available entity
 * classes which the EntityCreator can insert into a map.
 */

struct EntityClassManager
{
  INTEGER_CONSTANT(Version, 1);
  STRING_CONSTANT(Name, "eclassmanager");

  IEntityClass* (*findOrInsert)(const char* name, bool has_brushes);
  const ListAttributeType* (*findListType)(const char* name);
  void (*forEach)(EntityClassVisitor& visitor);
  void (*attach)(ModuleObserver& observer);
  void (*detach)(ModuleObserver& observer);
  void (*realise)();
  void (*unrealise)();
};

template<typename Type>
class GlobalModule;
typedef GlobalModule<EntityClassManager> GlobalEntityClassManagerModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<EntityClassManager> GlobalEntityClassManagerModuleRef;

inline EntityClassManager& GlobalEntityClassManager()
{
  return GlobalEntityClassManagerModule::getTable();
}

#endif
