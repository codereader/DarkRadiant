#pragma once

#include "imodule.h"
#include "idecltypes.h"

namespace decl
{

// Represents a declaration block as found in the various decl files
// Holds the name of the block, its typename and the raw block contents
// including whitespace and comments but exluding the outermost brace pair
struct DeclarationBlockSyntax
{
    // The type name of this block (e.g. "table")
    std::string typeName;

    // The name of this block (e.g. "sinTable" or "textures/common/caulk"
    std::string name;

    // The block contents (excluding braces)
    std::string contents;
};

// Common interface shared by all the declarations supported by a certain game type
class IDeclaration
{
public:
    virtual ~IDeclaration()
    {}

    using Ptr = std::shared_ptr<IDeclaration>;

    // The full name of this declaration, e.g. "textures/common/caulk"
    virtual const std::string& getName() const = 0;

    // The type of this declaration
    virtual Type getType() const = 0;
};

// Interface of a parser that can handle a single declaration type
class IDeclarationParser
{
public:
    virtual ~IDeclarationParser()
    {}

    using Ptr = std::shared_ptr<IDeclarationParser>;

    // Returns the declaration type this parser can handle
    virtual Type getDeclType() const = 0;

    // Create a new declaration instance from the given block
    virtual IDeclaration::Ptr parseFromBlock(const DeclarationBlockSyntax& block) = 0;
};

// Central service class holding all the declarations in the active game,
// like entityDefs, materials, skins, etc.
// Searches registered VFS folders and parses the contents of the matching files
// making use of the associated IDeclarationParser instances.
class IDeclarationManager :
    public RegisterableModule
{
public:
    virtual ~IDeclarationManager() override
    {}

    // Registers the declaration typename (e.g. "material") and associates it with the given parser
    // It's not allowed to register more than one parser for a single typename
    virtual void registerDeclType(const std::string& typeName, const IDeclarationParser::Ptr& parser) = 0;

    // Unregisters the given typename and the associated parser
    virtual void unregisterDeclType(const std::string& typeName) = 0;

    // Associates the given VFS folder path to a certain declaration type
    // all files matching the given file extension will be searched and parsed.
    // The folder will not be recursively searched for files, only immediate children will be parsed.
    // Any untyped declaration blocks found in the files will be assumed to be of the given defaultType.
    // All explicitly typed resources will be processed using the parser that has been previously
    // associated in registerDeclType()
    virtual void registerDeclFolder(Type defaultType, const std::string& vfsFolder, const std::string& extension) = 0;
};

}

constexpr const char* const MODULE_DECLMANAGER("DeclarationManager");

inline decl::IDeclarationManager& GlobalDeclarationManager()
{
    static module::InstanceReference<decl::IDeclarationManager> _reference(MODULE_DECLMANAGER);
    return _reference;
}
