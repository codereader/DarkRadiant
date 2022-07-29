/**
 * \defgroup eclass Entity class manager
 * \file ieclass.h
 * \brief Entity Class definition loader API.
 * \ingroup eclass
 */
#pragma once

#include "ideclmanager.h"
#include "igameresource.h"

#include "imodule.h"
#include "ifilesystem.h"
#include "math/Vector4.h"

#include <vector>
#include <list>
#include <map>
#include <memory>
#include <sigc++/signal.h>

/* FORWARD DECLS */

class Shader;
typedef std::shared_ptr<Shader> ShaderPtr;
class AABB;

/**
 * Data structure representing a single attribute on an entity class.
 *
 * \ingroup eclass
 */
class EntityClassAttribute
{
private:

    // Attribute type
    std::string _type;

    // Attribute name
    std::string _name;

    // Value
    std::string _value;

    //  User-friendly description
    std::string _desc;

public:
    /**
     * The key type (string, bool etc.).
     */
    const std::string& getType() const
    {
        return _type;
    }

    void setType(const std::string& type)
    {
        _type = type;
    }

    /// The attribute key name, e.g. "model", "editor_displayFolder" etc
    const std::string& getName() const
    {
        return _name;
    }

    /// Get attribute value
    const std::string& getValue() const
    {
        return _value;
    }

    /// Set attribute value
    void setValue(const std::string& value)
    {
        _value = value;
    }

    /// The help text associated with the key (in the DEF file).
    const std::string& getDescription() const
    {
        return _desc;
    }

    void setDescription(const std::string& desc)
    {
        _desc = desc;
    }

    /// Construct an EntityClassAttribute
    EntityClassAttribute(const std::string& type_,
                         const std::string& name_,
                         const std::string& value_,
                         const std::string& description_ = "")
    : _type(type_),
      _name(name_),
      _value(value_),
      _desc(description_)
    {}
};

/**
 * IEntityClass shared pointer.
 */
class IEntityClass;
typedef std::shared_ptr<IEntityClass> IEntityClassPtr;
typedef std::shared_ptr<const IEntityClass> IEntityClassConstPtr;

/**
 * \brief Entity class interface.
 *
 * An entity class represents a single type of entity that can be created by
 * the EntityCreator. Entity classes are parsed from .DEF files during startup.
 *
 * Entity class attribute names are compared case-insensitively, as in the
 * Entity class.
 *
 * \ingroup eclass
 */
class IEntityClass :
    public decl::IDeclaration
{
public:
    virtual ~IEntityClass() {}

    // Enumeration of types DarkRadiant is capable of distinguishing when creating entities
    enum class Type
    {
        Generic,            // fixed-size, coloured boxes with and without arrow
        StaticGeometry,     // func_* entities supporting primitives (like worldspawn)
        EntityClassModel,   // non-fixed size entities with a non-empty "model" key set
        Light,              // all classes with editor_light/idLight or inheriting from them
        Speaker,            // special class used for "speaker" entityDefs
    };

    // Returns the type of this entity class (as determined after parsing)
    virtual Type getClassType() = 0;

    /// Signal emitted when entity class contents are changed or reloaded
    virtual sigc::signal<void>& changedSignal() = 0;

    /// Get the parent entity class or NULL if there is no parent
    virtual IEntityClass* getParent() = 0;

    /// Get the UI visibility of this entity class
    virtual vfs::Visibility getVisibility() = 0;

    /// Query whether this entity class represents a light.
    virtual bool isLight() = 0;

    /* ENTITY CLASS SIZE */

    /// Query whether this entity has a fixed size.
    virtual bool isFixedSize() = 0;

    /**
     * Return an AABB representing the declared size of this entity. This is
     * only valid for fixed size entities.
     *
     * @returns
     * AABB enclosing the "editor_mins" and "editor_maxs" points defined in the
     * entityDef.
     */
    virtual AABB getBounds() = 0;

    /* ENTITY CLASS COLOURS */

    /// Return the display colour of this entity class
    virtual const Vector4& getColour() = 0;

    // Overrides the colour defined in the .def files
    virtual void setColour(const Vector4& colour) = 0;

    /* ENTITY CLASS ATTRIBUTES */

    /**
     * @brief Get the value of a specified attribute.
     *
     * @return std::string containing the attribute value, or an empty string if the attribute was
     * not found.
     */
    virtual std::string getAttributeValue(const std::string& name,
                                          bool includeInherited = true) = 0;

    // Returns the attribute type string for the given name.
    // This method will walk up the inheritance hierarchy until it encounters a type definition.
    // If no type is found, an empty string will be returned.
    virtual std::string getAttributeType(const std::string& name) = 0;

    // Returns the attribute description string for the given name.
    // This method will walk up the inheritance hierarchy until it encounters a non-empty description.
    virtual std::string getAttributeDescription(const std::string& name) = 0;

    /**
     * Function that will be invoked by forEachAttribute.
     *
     * The function will be passed each EntityClassAttribute in turn, along
     * with a bool indicating if this attribute is inherited from a parent
     * entity class.
     */
    using AttributeVisitor = std::function<void(const EntityClassAttribute&, bool)>;

    /**
     * Enumerate the EntityClassAttibutes in turn, including all inherited
     * attributes.
     *
     * \param visitor
     * Function that will be invoked for each EntityClassAttibute.
     *
     * \param editorKeys
     * true if editor keys (those which start with "editor_") should be passed
     * to the visitor, false if they should be skipped.
     */
    virtual void forEachAttribute(AttributeVisitor visitor, bool editorKeys = false) = 0;

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
    public decl::IDeclaration
{
public:
    using Ptr = std::shared_ptr<IModelDef>;

    // The def this model is inheriting from (empty if there's no parent)
    virtual const Ptr& getParent() = 0;

    // The MD5 mesh used by this modelDef
    virtual const std::string& getMesh() = 0;

    // The named skin
    virtual const std::string& getSkin() = 0;

    // The md5anim file name for the given anim key (e.g. "idle" or "af_pose")
    virtual std::string getAnim(const std::string& animKey) = 0;

    // Returns a dictionary of all the animations declared on this model def
    using Anims = std::map<std::string, std::string>;
    virtual const Anims& getAnims() = 0;
};

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

constexpr const char* const MODULE_ECLASSMANAGER("EntityClassManager");

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
    virtual IEntityClassPtr findClass(const std::string& name) = 0;

    /**
     * Iterate over all entity defs using the given visitor.
     */
    virtual void forEachEntityClass(EntityClassVisitor& visitor) = 0;

    // Iterate over all entityDefs using the given function object
    virtual void forEachEntityClass(const std::function<void(const IEntityClassPtr&)>& functor) = 0;

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
    virtual IModelDef::Ptr findModel(const std::string& name) = 0;

    /**
     * Iterate over each ModelDef using the given function object.
     */
    virtual void forEachModelDef(const std::function<void(const IModelDef::Ptr&)>& functor) = 0;
};

/**
 * Return the global EntityClassManager to the application.
 *
 * \ingroup eclass
 */
inline IEntityClassManager& GlobalEntityClassManager()
{
    static module::InstanceReference<IEntityClassManager> _reference(MODULE_ECLASSMANAGER);
    return _reference;
}
