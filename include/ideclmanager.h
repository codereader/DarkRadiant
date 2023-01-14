#pragma once

#include <map>
#include <sigc++/signal.h>
#include "imodule.h"
#include "ifilesystem.h"
#include "igameresource.h"
#include "idecltypes.h"

namespace decl
{

// Represents a declaration block as found in the various decl files
// Holds the name of the block, its typename and the raw block contents
// including whitespace and comments but exluding the outermost brace pair
struct DeclarationBlockSyntax : game::IResource
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
class IDeclaration :
    public game::IResource
{
public:
    virtual ~IDeclaration()
    {}

    using Ptr = std::shared_ptr<IDeclaration>;

    // The full name of this declaration, e.g. "textures/common/caulk"
    virtual const std::string& getDeclName() const = 0;

    // Change the name of this decl. Don't use this directly in client code, use
    // GlobalDeclarationManager().renameDeclaration() instead.
    virtual void setDeclName(const std::string& newName) = 0;

    // The original full name of this declaration, e.g. "textures/common/caulk",
    // as parsed from the decl file. This is used internally for saving/deleting
    // declarations, client code usually wants to use getDeclName().
    virtual const std::string& getOriginalDeclName() const = 0;

    // Update the name of this decl as it appears in the decl file
    // Used internally after saving a renamed decl.
    virtual void setOriginalDeclName(const std::string& newName) = 0;

    // The type of this declaration
    virtual Type getDeclType() const = 0;

    // The raw syntax block (without the outer curly braces) used to construct this decl
    virtual const DeclarationBlockSyntax& getBlockSyntax() = 0;

    // Set the block contents of this declaration.
    // Implementations are free to either (re-)parse immediately or deferred.
    virtual void setBlockSyntax(const DeclarationBlockSyntax& block) = 0;

    // Returns the mod-relative path to the file this decl has been declared in
    virtual std::string getDeclFilePath() const = 0;

    // Sets the file info as contained in the block syntax
    virtual void setFileInfo(const vfs::FileInfo& fileInfo) = 0;

    // Returns the value of the internally used parse epoch counter
    virtual std::size_t getParseStamp() const = 0;

    // Sets the internally used parse epoch counter 
    virtual void setParseStamp(std::size_t parseStamp) = 0;

    // Fired when this declaration changed (i.e. as result of a reloadDecls
    // operation or a change in an editor).
    virtual sigc::signal<void>& signal_DeclarationChanged() = 0;
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
    // Note: The given typename will be treated case-insensitively by the decl parser.
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

    // Find the declaration with the given type and name, or creates a default declaration
    // of the given type if nothing was found. Will always return a non-empty reference.
    // Throws std::invalid_argument exception of the type has not even been registered
    virtual IDeclaration::Ptr findOrCreateDeclaration(Type type, const std::string& name) = 0;

    // Iterate over all known declarations, using the given visitor
    virtual void foreachDeclaration(Type type, const std::function<void(const IDeclaration::Ptr&)>& functor) = 0;

    // Renames the declaration from oldName to newName. The new name must not be in use by any other declaration,
    // and it must be different from oldName, otherwise renaming will fail.
    // Returns true if the old declaration existed and could successfully be renamed, false on any failure.
    virtual bool renameDeclaration(Type type, const std::string& oldName, const std::string& newName) = 0;

    // Removes the given declaration both from memory and the decl file, if there is one.
    // Only declarations stored in physical files (or unsaved ones) can be removed.
    // Attempting to remove a read-only declaration (e.g. decls saved in PK4 archives)
    // will cause a std::logic_error to be thrown.
    virtual void removeDeclaration(Type type, const std::string& name) = 0;

    // Re-load all declarations.
    // All declaration references will stay intact, only their contents will be refreshed
    virtual void reloadDeclarations() = 0;

    // Saves the given declaration to a physical declaration file. Depending on the original location
    // of the declaration the outcome will be different.
    //
    // It is required for the declaration to have valid file information set on its syntax block,
    // otherwise the declaration manager will not know where to save it to and throws std::invalid_argument.
    //
    // Case #1: Newly created declarations (created through findOrCreateDeclaration):
    //      The decl will be appended to the given file. The file will be created if required.
    // Case #2: Existing declarations (with their original file being a physical file):
    //      The decl in the file will be replaced with its new syntax block. All the content before and
    //      after the declaration will be left untouched.
    // Case #3: Existing declarations (with their original file being stored within a PK4):
    //      The decl file will be copied and saved to its corresponding physical location, effectively
    //      creating a file that will override the one in the PK4. The decl will be merged into the file
    //      just like in case #2
    virtual void saveDeclaration(const IDeclaration::Ptr& decl) = 0;

    // Signal emitted right before decls are being reloaded
    virtual sigc::signal<void>& signal_DeclsReloading(Type type) = 0;

    // Signal emitted when the decls of the given type have been (re-)loaded
    // Note that this signal can be fired on an arbitrary thread
    virtual sigc::signal<void>& signal_DeclsReloaded(Type type) = 0;

    // Signal emitted when a declaration is renamed
    // The type, the old name and the new name will be passed as arguments
    virtual sigc::signal<void(Type, const std::string&, const std::string&)>& signal_DeclRenamed() = 0;

    // Signal emitted when a declaration has been created (e.g. by findOrCreateDeclaration),
    // passing the type and name of the created decl as argument
    virtual sigc::signal<void(Type, const std::string&)>& signal_DeclCreated() = 0;

    // Signal emitted when a declaration has been removed (by removeDeclaration),
    // passing the type and name of the removed decl as argument
    virtual sigc::signal<void(Type, const std::string&)>& signal_DeclRemoved() = 0;
};

}

constexpr const char* const MODULE_DECLMANAGER("DeclarationManager");

inline decl::IDeclarationManager& GlobalDeclarationManager()
{
    static module::InstanceReference<decl::IDeclarationManager> _reference(MODULE_DECLMANAGER);
    return _reference;
}
