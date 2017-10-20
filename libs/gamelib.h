#pragma once

#include "igame.h"
#include "registry/registry.h"
#include "string/convert.h"
#include "os/path.h"

namespace game
{

namespace current
{

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
	xml::NodeList list = GlobalGameManager().currentGame()->getLocalXPath(localXPath);

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

} // namespace

} // namespace
