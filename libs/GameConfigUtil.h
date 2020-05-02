#pragma once

#include "igame.h"

#include "registry/registry.h"
#include "os/path.h"
#include "os/file.h"

namespace game
{

/**
 * Some algorithms built around the GameConfiguration class.
 */
class GameConfigUtil
{
public:
	// Loads the property values of this instance from the XMLRegistry
	static GameConfiguration LoadFromRegistry()
	{
		GameConfiguration result;
		
		result.gameType = registry::getValue<std::string>(RKEY_GAME_TYPE);
		result.enginePath = os::standardPathWithSlash(registry::getValue<std::string>(RKEY_ENGINE_PATH));
		result.modPath = os::standardPathWithSlash(registry::getValue<std::string>(RKEY_MOD_PATH));
		result.modBasePath = os::standardPathWithSlash(registry::getValue<std::string>(RKEY_MOD_BASE_PATH));

		return result;
	}

	// Persists the values of this instance to the XMLRegistry
	static void SaveToRegistry(const GameConfiguration& config)
	{
		registry::setValue(RKEY_GAME_TYPE, config.gameType);
		registry::setValue(RKEY_ENGINE_PATH, config.enginePath);
		registry::setValue(RKEY_MOD_PATH, config.modPath);
		registry::setValue(RKEY_MOD_BASE_PATH, config.modBasePath);
	}

	// Makes sure that all paths are normalised OS paths
	static void EnsurePathsNormalised(GameConfiguration& config)
	{
		config.enginePath = os::standardPathWithSlash(config.enginePath);
		config.modPath = os::standardPathWithSlash(config.modPath);
		config.modBasePath = os::standardPathWithSlash(config.modBasePath);
	}

	// Returns true if the paths in this config are pointing to valid OS folders
	static bool PathsValid(const GameConfiguration& config)
	{
		if (!os::fileOrDirExists(config.enginePath))
		{
			// Engine path doesn't exist
			return false;
		}

		// Check the mod base path, if appropriate
		if (!config.modBasePath.empty() && !os::fileOrDirExists(config.modBasePath))
		{
			// Mod base name is not empty, but folder doesnt' exist
			return false;
		}

		// Check the mod path, if appropriate
		if (!config.modPath.empty() && !os::fileOrDirExists(config.modPath))
		{
			// Mod name is not empty, but mod folder doesnt' exist
			return false;
		}

		return true; // all paths ok
	}
};

}
