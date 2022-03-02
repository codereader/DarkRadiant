/**
 * \defgroup eclass Entity class manager
 * \file ieclass.h
 * \brief Entity Class definition loader API.
 * \ingroup eclass
 */
#pragma once

#include "ModResource.h"

#include "imodule.h"
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
    public ModResource
{
public:
    virtual ~IEntityClass() {}

    /// Signal emitted when entity class contents are changed or reloaded
    virtual sigc::signal<void>& changedSignal() = 0;

    /// Get the name of this entity class
    virtual const std::string& getName() const = 0;

    /// Get the parent entity class or NULL if there is no parent
    virtual const IEntityClass* getParent() const = 0;

    /// Query whether this entity class represents a light.
    virtual bool isLight() const = 0;

    /* ENTITY CLASS SIZE */

    /// Query whether this entity has a fixed size.
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
    virtual const Vector4& getColour() const = 0;

    // Overrides the colour defined in the .def files
    virtual void setColour(const Vector4& colour) = 0;

    /* ENTITY CLASS ATTRIBUTES */

    /**
     * Return a single named EntityClassAttribute from this EntityClass.
     *
     * \param name
     * The name of the EntityClassAttribute to find, interpreted case-insensitively.
     *
     * \param includeInherited
     * true if attributes inherited from parent entity classes should be
     * considered, false otherwise.
     *
     * \return
     * A reference to the named EntityClassAttribute. If the named attribute is
     * not found, an empty EntityClassAttribute is returned.
     */
    virtual EntityClassAttribute& getAttribute(const std::string& name, 
        bool includeInherited = true) = 0;

    /// Get a const EntityClassAttribute reference by name
    virtual const EntityClassAttribute& getAttribute(const std::string& name,
                 bool includeInherited = true) const = 0;

    // Returns the attribute type string for the given name.
    // This method will walk up the inheritance hierarchy until it encounters a type definition.
    // If no type is found, an empty string will be returned.
    virtual const std::string& getAttributeType(const std::string& name) const = 0;

    // Returns the attribute description string for the given name.
    // This method will walk up the inheritance hierarchy until it encounters a non-empty description.
    virtual const std::string& getAttributeDescription(const std::string& name) const = 0;

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
    virtual void forEachAttribute(AttributeVisitor visitor,
                                  bool editorKeys = false) const = 0;

    /* MODEL AND SKIN */

    /** Retrieve the model path for this entity.
     *
     * @returns
     * The VFS model path, or the empty string if there is no model.
     */
    virtual const std::string& getModelPath() const = 0;

    /// Get the model skin, or the empty string if there is no skin.
    virtual const std::string& getSkin() const = 0;

	/**
	 * Returns true if this entity is of type or inherits from the
	 * given entity class name. className is treated case-sensitively.
	 */
	virtual bool isOfType(const std::string& className) = 0;

    // Returns the mod-relative path to the file this DEF was declared in
    virtual std::string getDefFileName() = 0;
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

    // The mod-relative path to the file this DEF was declared in
    std::string defFilename;

    IModelDef() :
        resolved(false),
        modName("base")
    {}

    std::string getModName() const
    {
        return modName;
    }
};
typedef std::shared_ptr<IModelDef> IModelDefPtr;

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
    /// Signal emitted when starting to parse DEFs
    virtual sigc::signal<void>& defsLoadingSignal() = 0;

    /// Signal emitted when all DEFs have been loaded (after module initialisation)
    virtual sigc::signal<void>& defsLoadedSignal() = 0;

    /// Signal emitted when all DEFs are reloaded
    virtual sigc::signal<void>& defsReloadedSignal() = 0;

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
    virtual IModelDefPtr findModel(const std::string& name) = 0;

    /**
     * Iterate over each ModelDef using the given visitor class.
     */
    virtual void forEachModelDef(ModelDefVisitor& visitor) = 0;

    /**
     * Iterate over each ModelDef using the given function object.
     */
    virtual void forEachModelDef(const std::function<void(const IModelDefPtr&)>& functor) = 0;
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
