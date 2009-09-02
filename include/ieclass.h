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

/**
 * \defgroup eclass Entity class manager
 * \file ieclass.h
 * \brief Entity Class definition loader API.
 * \ingroup eclass
 */

#if !defined(INCLUDED_IECLASS_H)
#define INCLUDED_IECLASS_H

#include "ModResource.h"

#include "imodule.h"
#include "math/Vector3.h"

#include <vector>
#include <list>
#include <map>

/* FORWARD DECLS */

class Shader;
class ListAttributeType;
class AABB;

/** 
 * Data structure representing a single attribute on an entity class.
 * 
 * \ingroup eclass
 */
struct EntityClassAttribute
{
	/**
	 * The key type (string, bool etc.).
	 */
	std::string type;
	
	/**
	 * The attribute key name.
	 */
	std::string name;
	
	/**
	 * Current attribute value.
	 */
	std::string value;
	
	/**
	 * The help text associated with the key (in the DEF file).
	 */
	std::string description;

	/**
	 * Is TRUE for inherited keyvalues.
	 */
	bool inherited;

	/**
	 * Construct an EntityClassAttribute with empty strings and a false
	 * inherited flag.
	 */
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

/** 
 * Visitor class for EntityClassAttributes.
 * 
 * \ingroup eclass
 */
struct EntityClassAttributeVisitor {
	
	/** 
	 * Visit function.
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
typedef boost::shared_ptr<const IEntityClass> IEntityClassConstPtr;

/**
 * Entity class interface. An entity class represents a single type
 * of entity that can be created by the EntityCreator. Entity classes are parsed
 * from .DEF files during startup.
 * 
 * \ingroup eclass
 */
class IEntityClass 
: public ModResource
{
public:
	/**
	 * greebo: The inheritance chain, represented by a list of classnames, 
	 *         starting with the topmost class.
	 *
	 * Example: "atdm:ai_base" | "atdm:ai_humanoid" | "atdm:ai_builder_guard"
	 */ 
	typedef std::list<std::string> InheritanceChain;

	// An EntityClassObserver gets notified whenever the eclass contents 
	// get changed, possibly due to a reload of the declarations in the .def files.
	class Observer
	{
	public:
		/**
		 * greebo: Gets called as soon as the contents of the eclass
		 * get changed, which is true for the "reloadDefs" command.
		 */
		virtual void OnEClassReload() = 0;
	};
    
	// Adds/removes an eclass observer
	virtual void addObserver(Observer* observer) = 0;
	virtual void removeObserver(Observer* observer) = 0;

	/** 
	 * Get this entity class' name.
	 */
	virtual const std::string& getName() const = 0;

	/** 
	 * Query whether this entity class represents a light.
	 */
	virtual bool isLight() const = 0;
	
	/** 
	 * Set whether this entity class represents a light.
	 */
	virtual void setIsLight(bool isLight) = 0;

	/* ENTITY CLASS SIZE */

	/** 
	 * Query whether this entity has a fixed size.
	 */
	virtual bool isFixedSize() const = 0;

	/** 
	 * Return an AABB representing the declared size of this entity. This is
	 * only valid for fixed size entities.
	 * 
	 * @returns
	 * AABB enclosing the "editor_mins" and "editor_maxs" points defined in the 
	 * entityDef.
	 */
	virtual AABB getBounds() const = 0;
	
	/* ENTITY CLASS COLOURS */		

	/** 
	 * Set this entity class' display colour.
	 * 
	 * @param col
	 * Vector3 containing the R,G,B values to use.
	 */
	virtual void setColour(const Vector3& col) = 0;
	
	/** 
	 * Return this entity class' display colour.
	 * 
	 * @returns
	 * Vector3 reference containing the colour.
	 */
	virtual const Vector3& getColour() const = 0;
	
	/** 
	 * Get the Shader used for rendering this entity class in
	 * wireframe mode.
	 */
	virtual boost::shared_ptr<Shader> getWireShader() const = 0;
	
	/** 
	 * Get the Shader used for rendering this entity class in
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
	 * Return a single named EntityClassAttribute from this EntityClass. 
	 * 
	 * @param name
	 * The name of the EntityClassAttribute to find.
	 * 
	 * @return
	 * A reference to the named EntityClassAttribute. If the named attribute is 
	 * not found, an empty EntityClassAttribute is returned.
	 */
	virtual EntityClassAttribute& getAttribute(const std::string& name) = 0;
	virtual const EntityClassAttribute& getAttribute(const std::string& name) const = 0;
	
	/**
	 * Return the list of EntityClassAttributes matching the given prefix.
	 * 
	 * This method performs a search for all EntityClassAttributes whose name
	 * matches the given prefix, with a suffix consisting of zero or more 
	 * arbitrary characters. For example, if "target" were specified as the
	 * prefix, the list would include "target", "target0", "target127" etc.
	 * 
	 * This operation may not have high performance, due to the need to scan
	 * for matching names, therefore should not be used in performance-critical
	 * code.
	 * 
	 * @param name
	 * The prefix to search for.
	 * 
	 * @return
	 * A list of EntityClassAttribute objects matching the provided prefix. This
	 * list will be empty if there were no matches.
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

	/**
	 * greebo: Returns the list of base classes this eclass is inheriting from
	 *         including this eclass itself (as last element).
	 */
	virtual const InheritanceChain& getInheritanceChain() = 0;
};

/**
 * Structure ontains the information of a model {} block as defined in a 
 * Doom3 .def file.
 * 
 * \ingroup eclass
 */
class IModelDef :
	public ModResource
{
public:
	bool resolved;
	
	std::string name;
	
	std::string mesh;
	std::string skin;
	
	std::string parent;
	
	typedef std::map<std::string, std::string> Anims;
	Anims anims;

	std::string modName;
	
	IModelDef() : 
		resolved(false),
		modName("base")
	{}

	std::string getModName() const
	{
		return modName;
	}
};
typedef boost::shared_ptr<IModelDef> IModelDefPtr;

/** 
 * EntityClass visitor interface.
 * 
 * \ingroup eclass
 */
class EntityClassVisitor
{
public:
	virtual void visit(IEntityClassPtr eclass) = 0;
};

const std::string MODULE_ECLASSMANAGER("EntityClassManager");

/**
 * EntityClassManager interface. The entity class manager is responsible for 
 * maintaining a list of available entity classes which the EntityCreator can 
 * insert into a map.
 * 
 * \ingroup eclass
 */
class IEntityClassManager :
	public RegisterableModule
{
public:

	/**
	 * Return the IEntityClass corresponding to the given name, creating it if
	 * necessary. If it is created, the has_brushes parameter will be used to
	 * determine whether the new entity class should be brush-based or not.
     *
     * @deprecated
     * Use findClass() instead.
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

	virtual void realise() = 0;
	virtual void unrealise() = 0;

	/**
	 * greebo: This reloads the entityDefs and modelDefs from all files. Does not
	 * change the scenegraph, only the contents of the EClass objects are
	 * re-parsed. All IEntityClassPtrs remain valid, no entityDefs are removed.
	 *
	 * Note: This is NOT the same as unrealise + realise
	 */ 
	virtual void reloadDefs() = 0;
	
	/** greebo: Finds the model def with the given name. Might return NULL if not found.
	 */
	virtual IModelDefPtr findModel(const std::string& name) const = 0;
};

/**
 * Return the global EntityClassManager to the application.
 * 
 * \ingroup eclass
 */
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
