#pragma once

#include <map>
#include "imodule.h"
#include "ifilesystem.h"
#include "ModResource.h"
#include "idecltypes.h"

namespace decl
{

// Represents a declaration block as found in the various decl files
// Holds the name of the block, its typename and the raw block contents
// including whitespace and comments but exluding the outermost brace pair
struct DeclarationBlockSyntax : ModResource
{
    // The type name of this block (e.g. "table")
    std::string typeName;

    // The name of this block (e.g. "sinTable" or "textures/common/caulk"
    std::string name;

    // The block contents (excluding braces)
    std::string contents;

    // The mod this syntax has been defined in
    std::string modName;

    // The VFS info of the file this syntax is located
    vfs::FileInfo fileInfo;

    std::string getModName() const override
    {
        return modName;
    }
};

// Common interface shared by all the declarations supported by a certain game type
class IDeclaration
{
public:
    virtual ~IDeclaration()
    {}

    using Ptr = std::shared_ptr<IDeclaration>;

    // The full name of this declaration, e.g. "textures/common/caulk"
    virtual const std::string& getDeclName() const = 0;

    // The type of this declaration
    virtual Type getDeclType() const = 0;

    // The raw syntax block (without the outer curly braces) used to construct this decl
    virtual const DeclarationBlockSyntax& getBlockSyntax() const = 0;

    // Set the block contents of this declaration.
    // Implementations are free to either (re-)parse immediately or deferred.
    virtual void setBlockSyntax(const DeclarationBlockSyntax& block) = 0;

    // Returns the value of the internally used parse epoch counter
    virtual std::size_t getParseStamp() const = 0;

    // Sets the internally used parse epoch counter 
    virtual void setParseStamp(std::size_t parseStamp) = 0;
};

// Factory interface being able to create a single declaration type
class IDeclarationCreator
{
public:
    virtual ~IDeclarationCreator()
    {}

    using Ptr = std::shared_ptr<IDeclarationCreator>;

    // Returns the declaration type this creator can handle
    virtual Type getDeclType() const = 0;

    // Creates an empty declaration with the given name
    virtual IDeclaration::Ptr createDeclaration(const std::string& name) = 0;
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

    // Registers the declaration typename (e.g. "material") and associates it with the given creator
    // It's not allowed to register more than one creator for a single typename
    virtual void registerDeclType(const std::string& typeName, const IDeclarationCreator::Ptr& creator) = 0;

    // Unregisters the given typename and the associated creator
    virtual void unregisterDeclType(const std::string& typeName) = 0;

    // Associates the given VFS folder (with trailing slash) to a certain declaration type
    // all files matching the given file extension (without dot) will be searched and parsed.
    // The folder will not be recursively searched for files, only immediate children will be processed.
    // Any untyped declaration blocks found in the files will be assumed to be of the given defaultType.
    // All explicitly typed resources will be processed using the parser that has been previously
    // associated in registerDeclType()
    // Registering a folder will immediately trigger parsing of all contained files matching the criteria.
    virtual void registerDeclFolder(Type defaultType, const std::string& vfsFolder, const std::string& extension) = 0;

    // Find the declaration with the given type and name
    // Returns an empty reference if no declaration with that name could be found
    virtual IDeclaration::Ptr findDeclaration(Type type, const std::string& name) = 0;

    // Iterate over all known declarations, using the given visitor
    virtual void foreachDeclaration(Type type, const std::function<void(const IDeclaration::Ptr&)>& functor) = 0;

    // Re-load all declarations.
    // All declaration references will stay intact, only their contents will be refreshed
    virtual void reloadDeclarations() = 0;

    // Signal emitted when the decls of the given type have been (re-)loaded
    virtual sigc::signal<void>& signal_DeclsReloaded(Type type) = 0;
};

}

constexpr const char* const MODULE_DECLMANAGER("DeclarationManager");

inline decl::IDeclarationManager& GlobalDeclarationManager()
{
    static module::InstanceReference<decl::IDeclarationManager> _reference(MODULE_DECLMANAGER);
    return _reference;
}
