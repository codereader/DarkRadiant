#pragma once

#include "ideclmanager.h"
#include "ifilesystem.h"
#include "os/path.h"
#include "fmt/format.h"

namespace decl
{

/**
 * Verifies and extracts the relative save path from the the given full path to a decl file.
 * E.g. The full path "c:/games/darkmod/skins/test.skin" will be converted to "test.skin" when declFolder is "skins/".
 *
 * The declFolder is the folder relative to the mod path, like: "materials/"
 * The given path is verified to end with the given extension (with or without the dot, e.g. "mtr")
 *
 * Throws a std::invalid_argument exception if the given full path is not located in the given mod decl folder.
 */
inline std::string geRelativeDeclSavePath(const std::string& fullPath, const std::string& declFolder, std::string extension)
{
    std::string path = fullPath;

    if (path_is_absolute(path.c_str()))
    {
        auto rootPath = GlobalFileSystem().findRoot(path);

        if (rootPath.empty())
        {
            throw std::invalid_argument("The path " + path + " is not located in the current mod file structure");
        }

        path = os::getRelativePath(path, rootPath);
    }

    auto pathRelativeToDeclFolder = os::getRelativePath(path, os::standardPathWithSlash(declFolder));

    // Check if the path starts with a "decl/" folder
    // getRelativePath will return the unchanged path if this is not the case
    if (pathRelativeToDeclFolder == path)
    {
        throw std::invalid_argument("The path " + path + " does not point within " + os::standardPathWithSlash(declFolder) + " folder");
    }

    if (string::starts_with(extension, "."))
    {
        extension = extension.substr(1);
    }

    if (os::getExtension(pathRelativeToDeclFolder) != extension)
    {
        throw std::invalid_argument("The file extension must be " + extension);
    }

    return pathRelativeToDeclFolder;
}

// Returns a name that doesn't exist yet for this type
inline std::string generateNonConflictingName(Type type, const std::string& name)
{
    auto candidate = name;
    auto i = 0;

    while (GlobalDeclarationManager().findDeclaration(type, candidate))
    {
        candidate += fmt::format("{0:02d}", ++i);
    }

    return candidate;
}

}
