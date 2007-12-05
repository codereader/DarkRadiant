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

#include "imodule.h"
#include "math/Vector3.h"

#include <vector>
#include <map>

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

	// Is TRUE for inherited keyvalues.
	bool inherited;

	// Main constructor
	EntityClassAttribute(const std::string& t = "", 
						 const std::string& n = "", 
						 const std::string& v = "", 
						 const std::string& d = "") 
	: type(t), 
	  name(n), 
	  value(v), 
	  description(d), 
	  inherited(false)
	{}
};

/**
 * List of EntityClassAttributes.
 */
typedef std::vector<EntityClassAttribute> EntityClassAttributeList;

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
	 * Insert an EntityClassAttribute. If the attribute already exists, this
	 * has no effect.
	 */
	virtual void addAttribute(const EntityClassAttribute& attribute) = 0;

	/**
	 * Return a single named EntityClassAttribute from this EntityClass. If the
	 * named attribute is not found, an empty EntityClassAttribute is returned.
	 */
	virtual EntityClassAttribute& getAttribute(const std::string& name) = 0;
	virtual const EntityClassAttribute& getAttribute(const std::string& name) const = 0;
	
	/**
	 * Return the list of EntityClassAttributes matching the given name,
	 * including numbered suffixes (e.g. "target", "target1", "target2" etc).
	 * This list will be empty if no matching attributes were found.
	 * 
	 * This operation may not have high performance, due to the need to scan
	 * for matching names, therefore should not be used in performance-critical
	 * code.
	 */
	virtual EntityClassAttributeList getAttributeList(const std::string& name) 
	const = 0;

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

/**
 * greebo: A ModelDef contains the information of a model {} block 
 *         as defined in a Doom3 .def file.
 */
class IModelDef {
public:
	bool resolved;
	
	std::string name;
	
	std::string mesh;
	std::string skin;
	
	std::string parent;
	
	typedef std::map<std::string, std::string> Anims;
	Anims anims;
	
	IModelDef() : 
		resolved(false)
	{}
};
typedef boost::shared_ptr<IModelDef> IModelDefPtr;

/** EntityClass visitor interface.
 */

class EntityClassVisitor
{
public:
	virtual void visit(IEntityClassPtr eclass) = 0;
};

class ModuleObserver;

const std::string MODULE_ECLASSMANAGER("EntityClassManager");

/**
 * EntityClassManager interface. The entity class manager is responsible for 
 * maintaining a list of available entity classes which the EntityCreator can 
 * insert into a map.
 */
class IEntityClassManager :
	public RegisterableModule
{
public:
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
	
	/** greebo: Finds the model def with the given name. Might return NULL if not found.
	 */
	virtual IModelDefPtr findModel(const std::string& name) const = 0;
};

inline IEntityClassManager& GlobalEntityClassManager() {
	// Cache the reference locally
	static IEntityClassManager& _eclassMgr(
		*boost::static_pointer_cast<IEntityClassManager>(
			module::GlobalModuleRegistry().getModule(MODULE_ECLASSMANAGER)
		)
	);
	return _eclassMgr;
}

#endif
