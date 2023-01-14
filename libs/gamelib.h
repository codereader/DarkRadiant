#pragma once

#include "igame.h"
#include "itextstream.h"
#include "registry/registry.h"
#include "string/convert.h"
#include "os/path.h"

namespace game
{

namespace current
{

/**
 * \brief
 * Get the node(s) identified by the given path in the current game tree.
 */
inline xml::NodeList getNodes(const std::string& localPath)
{
	return GlobalGameManager().currentGame()->getLocalXPath(localPath);
}

/**
 * \brief
 * Get the value of the current game's tree in the registry and convert it 
 * to type T. If the key cannot be found or is not convertible to the 
 * required type, a default-constructed T will be returned.
 *
 * T must be default-constructible, copy-constructible and convertible from
 * an std::string using string::convert.
 */
template<typename T> 
inline T getValue(const std::string& localXPath, T defaultVal = T())
{
	xml::NodeList list = getNodes(localXPath);
	return list.empty() ? defaultVal : string::convert<T>(list[0].getAttributeValue("value"));
}

/**
 * Returns the current "mod" part of the full path. For Doom3-style games this is the part
 * relative to the engine path. For TDM-style games there's not necessarily a fs_game or 
 * fs_game_base set, in this case we fall back to the current game's name.
 */
inline std::string getModPath(const std::string& fullPath)
{
    std::string relPath = os::getRelativePathMinusFilename(fullPath, registry::getValue<std::string>(RKEY_ENGINE_PATH));

    // For the TDM game we don't necessarily have a "base" or "fs_game" directory
    // Fall back to the game name instead
    if (relPath.empty())
    {
        return GlobalGameManager().currentGame()->getKeyValue("name");
    }

    return relPath;
}

// Returns the extension of the auxiliary info file (including the leading dot character)
inline std::string getInfoFileExtension()
{
    constexpr const char* const GKEY_INFO_FILE_EXTENSION = "/mapFormat/infoFileExtension";

    auto extension = getValue<std::string>(GKEY_INFO_FILE_EXTENSION);

    if (!extension.empty() && extension[0] != '.')
    {
        extension = "." + extension;
    }

    return extension;
}

constexpr const char* const LIGHT_PREFIX_XPATH = "/light/texture//prefix";

/**
 * Returns the list of light texture prefixes, including trailing slashes
 */
inline std::vector<std::string> getLightTexturePrefixes()
{
    std::vector<std::string> prefixes;

    // Get the list of light texture prefixes from the registry
    auto prefList = getNodes(LIGHT_PREFIX_XPATH);

    // Copy the Node contents into the prefix vector
    for (const auto& node : prefList)
    {
        prefixes.push_back(node.getContent() + "/");
    }

    return prefixes;
}

/**
 * Returns the path where DarkRadiant is able to write custom game resources (like decls) to.
 * This defaults to the mod path and falls back to the user engine path if not found.
 * Does not create any directories.
 */
inline std::string getWriteableGameResourcePath()
{
    // Try the mod path first
    auto targetPath = GlobalGameManager().getModPath();

    if (targetPath.empty())
    {
        targetPath = GlobalGameManager().getUserEnginePath();

        rMessage() << "No mod path found, falling back to user engine path: " << targetPath << std::endl;
    }

    return targetPath;
}

} // namespace

} // namespace
