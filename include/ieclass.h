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
#pragma once

#include "ModResource.h"

#include "imodule.h"
#include "math/Vector3.h"

#include <vector>
#include <list>
#include <map>
#include <boost/shared_ptr.hpp>
#include <sigc++/signal.h>

/* FORWARD DECLS */

class Shader;
typedef boost::shared_ptr<Shader> ShaderPtr;
class AABB;

/**
 * Data structure representing a single attribute on an entity class.
 *
 * \ingroup eclass
 */
class EntityClassAttribute
{
private:
    /**
     * String references are shared_ptrs to save memory.
     * The actual string might be owned by another entity class we're inheriting from.
     */
    typedef boost::shared_ptr<std::string> StringPtr;

    // Reference to the name string
    StringPtr _typeRef;

    // Reference to the name string
    StringPtr _nameRef;

    // Reference to the attribute value string
    StringPtr _valueRef;

    // Reference to the description string
    StringPtr _descRef;

public:
    /**
     * The key type (string, bool etc.).
     */
    const std::string& getType() const
    {
        return *_typeRef;
    }

    const StringPtr& getTypeRef() const
    {
        return _typeRef;
    }

    void setType(const std::string& type)
    {
        _typeRef.reset(new std::string(type));
    }

    void setType(const StringPtr& typeRef)
    {
        _typeRef = typeRef;
    }

    /// The attribute key name, e.g. "model", "editor_displayFolder" etc
    const std::string& getName() const
    {
        return *_nameRef;
    }

    const StringPtr& getNameRef() const
    {
        return _nameRef;
    }

    /**
     * Direct reference to the value for easy access to the value. This reference
     * is pointing directly at the string owned by the ValueRef shared_ptr,
     * which in turn might be owned by a class we're inheriting from.
     */
    const std::string& getValue() const
    {
        return *_valueRef;
    }

    const StringPtr& getValueRef() const
    {
        return _valueRef;
    }

    /**
     * Sets the value of this entity class attribute. This will break up any
     * inheritance and make this instance owner of its value string.
     */
    void setValue(const std::string& value)
    {
        _valueRef.reset(new std::string(value));
    }

    void setValue(const StringPtr& valueRef)
    {
        _valueRef = valueRef;
    }

    /**
     * The help text associated with the key (in the DEF file).
     */
    const std::string& getDescription() const
    {
        return *_descRef;
    }

    const StringPtr& getDescriptionRef() const
    {
        return _descRef;
    }

    void setDescription(const std::string& desc)
    {
        _descRef.reset(new std::string(desc));
    }

    void setDescription(const StringPtr& descRef)
    {
        _descRef = descRef;
    }

    /**
     * Is TRUE for inherited keyvalues.
     */
    bool inherited;

    /**
     * Construct a non-inherited EntityClassAttribute, passing the actual strings
     * which will be owned by this class instance.
     */
    EntityClassAttribute(const std::string& type_,
                         const std::string& name_,
                         const std::string& value_, 
                         const std::string& description_ = "")
    : _typeRef(new std::string(type_)),
      _nameRef(new std::string(name_)),
      _valueRef(new std::string(value_)),
      _descRef(new std::string(description_)),
      inherited(false)
    {}

    /**
     * Construct a inherited EntityClassAttribute with a true inherited flag.
     * The strings are taken from the inherited attribute.
     * Note: this is not a copy-constructor on purpose, to allow STL assignments to 
     * copy the actual instance values.
     */
    EntityClassAttribute(const EntityClassAttribute& parentAttr, bool inherited_)
    : _typeRef(parentAttr._typeRef),    // take type string,
      _nameRef(parentAttr._nameRef),    // name string,
      _valueRef(parentAttr._valueRef),  // value string 
      _descRef(parentAttr._descRef),    // and description from the parent attribute
      inherited(inherited_)
    {}
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
 * Entity class attribute names are compared case-insensitively, as in the
 * Entity class.
 *
 * \ingroup eclass
 */
class IEntityClass
: public ModResource
{
public:

    /// Signal emitted when entity class contents are changed or reloaded
    virtual sigc::signal<void> changedSignal() const = 0;

    /// Get the name of this entity class
    virtual std::string getName() const = 0;

    /// Get the parent entity class or NULL if there is no parent
    virtual const IEntityClass* getParent() const = 0;

    /// Query whether this entity class represents a light.
    virtual bool isLight() const = 0;

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

    /// Return the display colour of this entity class
    virtual const Vector3& getColour() const = 0;

    /**
     * Get the named Shader used for rendering this entity class in
     * wireframe mode.
     */
    virtual const std::string& getWireShader() const = 0;

    /**
     * Get the Shader used for rendering this entity class in
     * filled mode.
     */
    virtual const std::string& getFillShader() const = 0;


    /* ENTITY CLASS ATTRIBUTES */

    /**
     * Return a single named EntityClassAttribute from this EntityClass.
     *
     * @param name
     * The name of the EntityClassAttribute to find, interpreted case-insensitively.
     *
     * @return
     * A reference to the named EntityClassAttribute. If the named attribute is
     * not found, an empty EntityClassAttribute is returned.
     */
    virtual EntityClassAttribute& getAttribute(const std::string& name) = 0;
    virtual const EntityClassAttribute& getAttribute(const std::string& name) const = 0;

    /**
     * Enumerate the EntityClassAttibutes in turn.
     *
     * \param visitor
     * Function that will be invoked for each EntityClassAttibute.
     *
     * \param editorKeys
     * true if editor keys (those which start with "editor_") should be passed
     * to the visitor, false if they should be skipped.
     */
    virtual void forEachClassAttribute(
        boost::function<void(const EntityClassAttribute&)> visitor,
        bool editorKeys = false
    ) const = 0;

    /* MODEL AND SKIN */

    /** Retrieve the model path for this entity.
     *
     * @returns
     * The VFS model path, or the empty string if there is no model.
     */
    virtual const std::string& getModelPath() const = 0;

    /** Get the model skin, or the empty string if there is no skin.
     */
    virtual const std::string& getSkin() const = 0;

	/**
	 * Returns true if this entity is of type or inherits from the 
	 * given entity class name. className is treated case-sensitively.
	 */
	virtual bool isOfType(const std::string& className) = 0;
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
    virtual ~EntityClassVisitor() {}
    virtual void visit(const IEntityClassPtr& eclass) = 0;
};

/**
 * ModelDef visitor interface.
 *
 * \ingroup eclass
 */
class ModelDefVisitor
{
public:
    virtual ~ModelDefVisitor() {}
    virtual void visit(const IModelDefPtr& modelDef) = 0;
};

const char* const MODULE_ECLASSMANAGER("EntityClassManager");

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

    /// Signal emitted when all DEFs are reloaded
    virtual sigc::signal<void> defsReloadedSignal() const = 0;

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

    /**
     * Iterate over all entity defs using the given visitor.
     */
    virtual void forEachEntityClass(EntityClassVisitor& visitor) = 0;

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

    /** 
     * greebo: Finds the model def with the given name. Might return NULL if not found.
     */
    virtual IModelDefPtr findModel(const std::string& name) const = 0;

    /**
     * Iterate over each ModelDef using the given visitor class.
     */
    virtual void forEachModelDef(ModelDefVisitor& visitor) = 0;
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
