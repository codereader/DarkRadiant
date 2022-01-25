#pragma once

#include "ieclass.h"
#include "irender.h"
#include "ifilesystem.h"

#include "math/Vector3.h"
#include "math/AABB.h"
#include "string/string.h"

#include "parser/DefTokeniser.h"

#include <vector>
#include <map>
#include <memory>

/* FORWARD DECLS */

class Shader;

namespace eclass
{

/// Implementation of the IEntityClass interface.
class EntityClass
: public IEntityClass
{
public:

    /// EntityClass pointer type
    using Ptr = std::shared_ptr<EntityClass>;

private:
    typedef std::shared_ptr<std::string> StringPtr;

    // The name of this entity class
    std::string _name;

    // Source file information
    vfs::FileInfo _fileInfo;

    // Parent class pointer (or NULL)
    EntityClass* _parent = nullptr;

    // Should this entity type be treated as a light?
    bool _isLight;

    // Colour of this entity and flag to indicate it has been specified
    Vector4 _colour;
    bool _colourTransparent;

    // Does this entity have a fixed size?
    bool _fixedSize;

    // Map of named EntityAttribute structures. EntityAttributes are picked
    // up from the DEF file during parsing. Ignores key case.
    typedef std::map<std::string, EntityClassAttribute, string::ILess> EntityAttributeMap;
    EntityAttributeMap _attributes;

    // The model and skin for this entity class (if it has one)
    std::string _model;
    std::string _skin;

    // Flag to indicate inheritance resolved. An EntityClass resolves its
    // inheritance by copying all values from the parent onto the child,
    // after recursively instructing the parent to resolve its own inheritance.
    bool _inheritanceResolved;

    // Name of the mod owning this class
    std::string _modName;

    // The empty attribute
    static const EntityClassAttribute _emptyAttribute;

    // The time this def has been parsed
    std::size_t _parseStamp;

    // Emitted when contents are reloaded
    sigc::signal<void> _changedSignal;
    bool _blockChangeSignal;

private:
    // Clear all contents (done before parsing from tokens)
    void clear();
    void parseEditorSpawnarg(const std::string& key, const std::string& value);
    void setIsLight(bool val);

    // Visit attributes recursively, parent first then child
    using InternalAttrVisitor = std::function<void(const EntityClassAttribute&)>;
    void forEachAttributeInternal(InternalAttrVisitor visitor,
                                  bool editorKeys) const;

public:
    /**
     * Static function to create a default entity class.
     *
     * @param name
     * The name of the entity class to create.
     *
     * @param brushes
     * Whether the entity contains brushes or not.
     */
    static EntityClass::Ptr create(const std::string& name, bool brushes);

    /**
     * Constructor.
     *
     * @param name
     * Entity class name.
     *
     * This eclass will have isFixedSize set to false.
     */
    EntityClass(const std::string& name, const vfs::FileInfo& fileInfo);

    /**
     * Constructor.
     *
     * @param name
     * Entity class name.
     *
     * @param fixedSize
     * whether this entity has a fixed size.
     */
    EntityClass(const std::string& name, const vfs::FileInfo& fileInfo, bool fixedSize);

    void emplaceAttribute(EntityClassAttribute&& attribute);

    // IEntityClass implementation
    const std::string& getName() const override;
    const IEntityClass* getParent() const override;
    sigc::signal<void>& changedSignal() override;
    bool isFixedSize() const override;
    AABB getBounds() const override;
    bool isLight() const override;
    const Vector4& getColour() const override;
    /// Set the display colour
    void setColour(const Vector4& colour) override;
    // Resets the colour to the value defined in the attributes
    void resetColour();
    EntityClassAttribute& getAttribute(const std::string&, bool includeInherited = true) override;
    const EntityClassAttribute& getAttribute(const std::string&, bool includeInherited = true) const override;
    const std::string& getAttributeType(const std::string& name) const override;
    const std::string& getAttributeDescription(const std::string& name) const override;
    void forEachAttribute(AttributeVisitor, bool) const override;

    const std::string& getModelPath() const override { return _model; }
    const std::string& getSkin() const override { return _skin; }

	bool isOfType(const std::string& className) override;

    std::string getDefFileName() override;

    /// Set a model on this entity class.
    void setModelPath(const std::string& path) {
        _fixedSize = true;
        _model = path;
    }

    /// Set the skin.
    void setSkin(const std::string& skin) { _skin = skin; }

    /**
     * Resolve inheritance for this class.
     *
     * @param classmap
     * A reference to the global map of entity classes, which should be searched
     * for the parent entity.
     */
    typedef std::map<std::string, EntityClass::Ptr> EntityClasses;
    void resolveInheritance(EntityClasses& classmap);

    /**
     * Return the mod name.
     */
    std::string getModName() const override {
        return _modName;
    }

    /**
     * Set the mod name.
     */
    void setModName(const std::string& mn) {
        _modName = mn;
    }

    // Initialises this class from the given tokens
    void parseFromTokens(parser::DefTokeniser& tokeniser);

    void setParseStamp(std::size_t parseStamp)
    {
        _parseStamp = parseStamp;
    }

    std::size_t getParseStamp() const
    {
        return _parseStamp;
    }

    void emitChangedSignal()
    {
        if (!_blockChangeSignal)
        {
            _changedSignal.emit();
        }
    }

    void blockChangedSignal(bool block)
    {
        _blockChangeSignal = block;
    }
};

}
