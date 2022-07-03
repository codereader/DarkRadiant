#pragma once

#include "ieclass.h"
#include "irender.h"
#include "ifilesystem.h"

#include "math/Vector3.h"
#include "math/AABB.h"
#include "string/string.h"
#include "generic/Lazy.h"

#include "parser/DefTokeniser.h"
#include "DeclarationBase.h"

#include <vector>
#include <map>
#include <memory>
#include <optional>

#include <sigc++/connection.h>

/* FORWARD DECLS */

class Shader;

namespace eclass
{

/// Implementation of the IEntityClass interface.
class EntityClass :
    public decl::DeclarationBase<IEntityClass>
{
public:

    /// EntityClass pointer type
    using Ptr = std::shared_ptr<EntityClass>;

private:

    // The name of this entity class
    std::string _name;

    // Source file information. May not exist if the entity class was created in code rather than
    // loaded from a .def file.
    std::optional<vfs::FileInfo> _fileInfo;

    // Parent class pointer (or NULL)
    EntityClass* _parent = nullptr;

    // UI visibility of this entity class
    Lazy<vfs::Visibility> _visibility;

    // Should this entity type be treated as a light?
    bool _isLight = false;

    // Colour of this entity
    Vector4 _colour;

    bool _colourTransparent = false;

    // Does this entity have a fixed size?
    bool _fixedSize;

    // Map of named EntityAttribute structures. EntityAttributes are picked
    // up from the DEF file during parsing. Ignores key case.
    using EntityAttributeMap = std::map<std::string, EntityClassAttribute, string::ILess>;
    EntityAttributeMap _attributes;

    // The model and skin for this entity class (if it has one)
    std::string _model;
    std::string _skin;

    // Flag to indicate inheritance resolved. An EntityClass resolves its
    // inheritance by copying all values from the parent onto the child,
    // after recursively instructing the parent to resolve its own inheritance.
    bool _inheritanceResolved = false;

    // Name of the mod owning this class
    std::string _modName = "base";

    // Emitted when contents are reloaded
    sigc::signal<void> _changedSignal;
    bool _blockChangeSignal = false;
    sigc::connection _parentChangedConnection;

    bool _parsed;

private:
    // Clear all contents (done before parsing from tokens)
    void clear();
    void parseEditorSpawnarg(const std::string& key, const std::string& value);
    void setIsLight(bool val);

    // Visit attributes recursively, parent first then child
    using InternalAttrVisitor = std::function<void(const EntityClassAttribute&)>;
    void forEachAttributeInternal(InternalAttrVisitor visitor,
                                  bool editorKeys) const;

    // Return attribute if found, possibly checking parents
    EntityClassAttribute* getAttribute(const std::string&, bool includeInherited = true);
    const EntityClassAttribute* getAttribute(const std::string&, bool includeInherited = true) const;

public:

    /// Construct an EntityClass with a given FileInfo
    EntityClass(const std::string& name, const vfs::FileInfo& fileInfo);

    ~EntityClass();

    /// Create a heap-allocated default/empty EntityClass
    static Ptr CreateDefault(const std::string& name);

    const std::string& getDeclName() const override
    {
        return getName();
    }

    decl::Type getDeclType() const override
    {
        return decl::Type::EntityDef;
    }

    void emplaceAttribute(EntityClassAttribute&& attribute);

    // IEntityClass implementation
    const std::string& getName() const override;
    const IEntityClass* getParent() const override;
    vfs::Visibility getVisibility() const override;
    sigc::signal<void>& changedSignal() override;
    bool isFixedSize() const override;
    AABB getBounds() const override;
    bool isLight() const override;
    const Vector4& getColour() const override;
    /// Set the display colour
    void setColour(const Vector4& colour) override;
    // Resets the colour to the value defined in the attributes
    void resetColour();
    std::string getAttributeValue(const std::string&, bool includeInherited = true) const override;
    std::string getAttributeType(const std::string& name) const override;
    std::string getAttributeDescription(const std::string& name) const override;
    void forEachAttribute(AttributeVisitor, bool) const override;

#if 0
    const std::string& getModelPath() const override { return _model; }
    const std::string& getSkin() const override { return _skin; }
#endif

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
    typedef std::map<std::string, Ptr> EntityClasses;
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

protected:
    void onSyntaxBlockAssigned(const decl::DeclarationBlockSyntax& block) override;
};

}
