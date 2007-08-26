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

#include "ModResource.h"

#include "generic/constant.h"
#include "math/Vector3.h"
#include "modulesystem.h"

#include <boost/shared_ptr.hpp>

/* FORWARD DECLS */

class Shader;
class ListAttributeType;
class AABB;

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

/** Visitor class for EntityClassAttributes.
 */
struct EntityClassAttributeVisitor {
	
	/** Visit function.
	 * 
	 * @param attr
	 * The current EntityClassAttribute to visit.
	 */
	virtual void visit(const EntityClassAttribute&) = 0;
};

/**
 * IEntityClass shared pointer.
 */
class IEntityClass;
typedef boost::shared_ptr<IEntityClass> IEntityClassPtr;

/*
 * * Entity class interface. An entity class represents a single type
 * of entity that can be created by the EntityCreator.
 */
class IEntityClass 
: public ModResource
{
public:
    
	/** 
	 * Get this entity class' name.
	 */
	virtual const std::string& getName() const = 0;

	/** Query whether this entity class represents a light.
	 */
	virtual bool isLight() const = 0;
	
	/** Set whether this entity class represents a light.
	 */
	virtual void setIsLight(bool isLight) = 0;

	/* ENTITY CLASS SIZE */

	/** Query whether this entity has a fixed size.
	 */
	virtual bool isFixedSize() const = 0;

	/** Return an AABB representing the declared size of this entity. This is
	 * only valid for fixed size entities.
	 * 
	 * @returns
	 * AABB enclosing the "editor_mins" and "editor_maxs" points defined in the 
	 * entityDef.
	 */
	virtual AABB getBounds() const = 0;
	

	/* ENTITY CLASS COLOURS */		

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
	virtual boost::shared_ptr<Shader> getWireShader() const = 0;
	
	/** Get the Shader used for rendering this entity class in
	 * filled mode.
	 */
	virtual boost::shared_ptr<Shader> getFillShader() const = 0;


	/* ENTITY CLASS ATTRIBUTES */		

	/** 
	 * Insert an EntityClassAttribute.
	 */
	virtual void addAttribute(const EntityClassAttribute& attribute) = 0;

	/**
	 * Find a named EntityClassAttribute. Throws an exception if the named
	 * attribute does not exist.
	 */
	virtual const EntityClassAttribute& 
		findAttribute(const std::string& name) const = 0;

	/** Return the value associated with a given entity class attribute.
	 * Any key may be looked up, including "editor_" keys.
	 * 
	 * @param key
	 * The key to lookup
	 * 
	 * @returns
	 * The string value of the key, or an empty string if the key is not
	 * found.
	 */
	virtual std::string getValueForKey(const std::string& key) const = 0;

	/** 
	 * Enumerate the EntityClassAttibutes in turn.
	 * 
	 * @param visitor
	 * An EntityClassAttributeVisitor instance.
	 * 
	 * @param editorKeys
	 * true if editor keys (those which start with "editor_") should be passed
	 * to the visitor, false if they should be skipped.
	 */
	virtual void forEachClassAttribute(EntityClassAttributeVisitor& visitor, 
									   bool editorKeys = false) const = 0;


	/* MODEL AND SKIN */

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
};

/** EntityClass visitor interface.
 */

class EntityClassVisitor
{
public:
	virtual void visit(IEntityClassPtr eclass) = 0;
};

class ModuleObserver;


/**
 * EntityClassManager interface. The entity class manager is responsible for 
 * maintaining a list of available entity classes which the EntityCreator can 
 * insert into a map.
 */
struct IEntityClassManager
{
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "eclassmanager");

	/**
	 * Return the IEntityClass corresponding to the given name, creating it if
	 * necessary. If it is created, the has_brushes parameter will be used to
	 * determine whether the new entity class should be brush-based or not.
	 */
	virtual IEntityClassPtr findOrInsert(const std::string& name, 
										 bool has_brushes) = 0;
	
    /**
     * Lookup an entity class by name. If the class is not found, a null pointer
     * is returned.
     *
     * @param name
     * Name of the entity class to look up.
     */
    virtual IEntityClassPtr findClass(const std::string& name) const = 0;
    
 	virtual void forEach(EntityClassVisitor& visitor) = 0;
	virtual void attach(ModuleObserver& observer) = 0;
	virtual void detach(ModuleObserver& observer) = 0;
	virtual void realise() = 0;
	virtual void unrealise() = 0;
};

template<typename Type>
class GlobalModule;
typedef GlobalModule<IEntityClassManager> GlobalEntityClassManagerModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<IEntityClassManager> GlobalEntityClassManagerModuleRef;

inline IEntityClassManager& GlobalEntityClassManager()
{
  return GlobalEntityClassManagerModule::getTable();
}

#endif
