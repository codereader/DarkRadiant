#pragma once

#include "ieclass.h"
#include "irender.h"
#include "ifilesystem.h"

#include "math/Vector3.h"
#include "math/AABB.h"
#include "string/string.h"
#include "generic/Lazy.h"

#include "parser/DefTokeniser.h"
#include "decl/DeclarationBase.h"

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

    // Flag to indicate inheritance resolved. An EntityClass resolves its
    // inheritance by copying all values from the parent onto the child,
    // after recursively instructing the parent to resolve its own inheritance.
    bool _inheritanceResolved = false;

    // Emitted when contents are reloaded
    sigc::signal<void> _changedSignal;
    bool _blockChangeSignal = false;
    sigc::connection _parentChangedConnection;

private:
    // Resolve inheritance for this class.
    void resolveInheritance();
    vfs::Visibility determineVisibilityFromValues();

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

public:

    /// Construct a named EntityClass
    EntityClass(const std::string& name);

    ~EntityClass();

    /// Create a heap-allocated default/empty EntityClass
    static Ptr CreateDefault(const std::string& name);

    Type getClassType() override;

    void emplaceAttribute(EntityClassAttribute&& attribute);

    // IEntityClass implementation
    IEntityClass* getParent() override;
    vfs::Visibility getVisibility() override;
    sigc::signal<void>& changedSignal() override;
    bool isFixedSize() override;
    AABB getBounds() override;
    bool isLight() override;
    const Vector4& getColour() override;
    /// Set the display colour
    void setColour(const Vector4& colour) override;
    // Resets the colour to the value defined in the attributes
    void resetColour();
    std::string getAttributeValue(const std::string&, bool includeInherited = true) override;
    std::string getAttributeType(const std::string& name) override;
    std::string getAttributeDescription(const std::string& name) override;
    void forEachAttribute(AttributeVisitor, bool) override;

	bool isOfType(const std::string& className) override;

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
    // Initialises this class from the given tokens
    void parseFromTokens(parser::DefTokeniser& tokeniser) override;

    // Clears the structure before parsing
    void onBeginParsing() override;

    // After parsing, inheritance and colour overrides will be resolved
    void onParsingFinished() override;

    void onSyntaxBlockAssigned(const decl::DeclarationBlockSyntax& block) override;
};

}
