#include "GameManager.h"

#include "i18n.h"
#include "iregistry.h"
#include "iradiant.h"
#include "imessagebus.h"
#include "ifiletypes.h"
#include "icommandsystem.h"
#include "itextstream.h"
#include "ifilesystem.h"
#include "ipreferencesystem.h"

#include "os/file.h"
#include "os/dir.h"
#include "os/path.h"
#include "os/fs.h"
#include "string/trim.h"
#include "string/convert.h"
#include "string/replace.h"
#include "string/predicate.h"
#include "string/split.h"
#include "string/case_conv.h"

#include "module/StaticModule.h"
#include "messages/GameConfigNeededMessage.h"

#include <sigc++/bind.h>
#include <fmt/format.h>

#include <iostream>

namespace game
{

namespace
{
	const char* const GKEY_PREFAB_FOLDER = "/mapFormat/prefabFolder";
	const char* const GKEY_MAPS_FOLDER = "/mapFormat/mapFolder";
    const char* const PAK_ICON = "package.png";
}

Manager::Manager()
{}

const std::string& Manager::getName() const
{
	static std::string _name(MODULE_GAMEMANAGER);
	return _name;
}

const StringSet& Manager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_FILETYPES);
	}

	return _dependencies;
}

void Manager::initialiseModule(const IApplicationContext& ctx)
{
	// Scan the <applicationpath>/games folder for .game files
	loadGameFiles(ctx.getRuntimeDataPath());

	// Check if there's a saved game type in the registry,
	// try to setup a default if nothing is found.
	initialiseGameType();

	// Try to retrieve a persisted game setup from the registry
	GameConfiguration config = GameConfigUtil::LoadFromRegistry();

	// Read command line parameters, these override any existing preference setting
	// but only if we have a valid engine path
	if (!config.enginePath.empty())
	{
		const auto& args = ctx.getCmdLineArgs();

		for (const std::string& arg : args)
		{
			if (string::istarts_with(arg, "fs_game="))
			{
				rMessage() << "Found fs_game command line argument, overriding existing mod path." << std::endl;

				// Remove starting slash from argument and convert to standard paths
				config.modPath = os::standardPathWithSlash(config.enginePath) +
					os::standardPath(string::trim_left_copy(arg.substr(8), "/"));

				registry::setValue(RKEY_MOD_PATH, config.modPath);
			}
			else if (string::istarts_with(arg, "fs_game_base="))
			{
				rMessage() << "Found fs_game_base command line argument, overriding existing mod base path." << std::endl;

				// Remove starting slash from argument and convert to standard paths
				config.modBasePath = os::standardPathWithSlash(config.enginePath) +
					os::standardPath(string::trim_left_copy(arg.substr(13), "/"));

				registry::setValue(RKEY_MOD_BASE_PATH, config.modBasePath);
			}
		}
	}

	// Check validity of the saved game configuration
	// and invoke the UI if it's not a valid one.
	if (GameConfigUtil::PathsValid(config))
	{
		applyConfig(config);
	}
	else
	{
		// The UI will call applyConfig on its own
		showGameSetupDialog();
	}

	// Add a legacy note to the preference dialog for folks who are looking for the old settings page
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Game"));
	page.appendLabel(_("This page has been moved!\nPlease use the game settings dialog in the menu: File &gt; Game/Project Setup..."));
}

const std::string& Manager::getModPath() const
{
	// Return the fs_game path if available
	return (!_config.modPath.empty()) ? _config.modPath : _config.modBasePath;
}

const std::string& Manager::getModBasePath() const
{
	return _config.modBasePath;
}

IGamePtr Manager::currentGame()
{
	if (_config.gameType.empty())
	{
		// No game type selected, bail out, the program will crash anyway on module load
		throw std::runtime_error(_("GameManager: No game type selected, can't continue."));
	}

	return _games[_config.gameType];
}

const Manager::GameList& Manager::getSortedGameList()
{
	return _sortedGames;
}

void Manager::initialiseGameType()
{
	if (_games.empty())
	{
		// No game types available, bail out, the program would crash anyway on module load
		throw std::runtime_error(_("GameManager: No valid game files found, can't continue."));
	}

	// Find the user's selected game from the XML registry, and attempt to find
	// it in the set of available games.
	std::string gameType = registry::getValue<std::string>(RKEY_GAME_TYPE);
	GameMap::const_iterator i = _games.find(gameType);

	if (gameType.empty() || i == _games.end())
   {
		// We have at least one game type available, select the first
		// Store the name of the only game into the Registry
		rMessage() << "No game selected, will choose the highest ranked one." << std::endl;

		if (_sortedGames.empty())
		{
			throw std::runtime_error("GameManager: Sorted game list is empty, can't continue.");
		}

		registry::setValue(RKEY_GAME_TYPE, _sortedGames.front()->getKeyValue("name"));
	}

	// Load the value from the registry, there should be one selected at this point
	_config.gameType = registry::getValue<std::string>(RKEY_GAME_TYPE);

	// The game type should be selected now
	if (!_config.gameType.empty())
    {
		rMessage() << "GameManager: Selected game type: " << _config.gameType << std::endl;
	}
	else
	{
		// No game type selected, bail out, the program would crash anyway on module load
		throw std::runtime_error(_("No game type selected."));
	}
}

std::string Manager::getUserEnginePath()
{
#if defined(POSIX)

    // First check for a local copy of the game tree, e.g. ~/.doom3
    std::string localGamePrefix = currentGame()->getKeyValue("prefix");
    if (!localGamePrefix.empty())
    {
        std::string homeDir = getenv("HOME");
        fs::path localPath = fs::path(homeDir) / localGamePrefix;

        if (fs::exists(localPath))
        {
            return os::standardPathWithSlash(localPath);
        }
    }

#endif

    // Otherwise (Windows, or no local mod path found) return the regular engine path
    return _config.enginePath;
}

const GameConfiguration& Manager::getConfig() const
{
    return _config;
}

void Manager::applyConfig(const GameConfiguration& config)
{
	if (!GameConfigUtil::PathsValid(config))
	{
		rError() << "GameManager: Cannot apply invalid configuration, paths not valid" << std::endl;
		return;
	}

	// Store the configuration, and persist it to the registry
	_config = config;
	GameConfigUtil::SaveToRegistry(_config);

	// Extract the fs_game / fs_game_base settings from the mod path
	std::string fsGame = os::getRelativePath(_config.modPath, _config.enginePath);
	string::trim_right(fsGame, "/");

	std::string fsGameBase = os::getRelativePath(_config.modBasePath, _config.enginePath);
	string::trim_right(fsGameBase, "/");

	registry::setValue(RKEY_FS_GAME, fsGame);
	registry::setValue(RKEY_FS_GAME_BASE, fsGameBase);

	// Instruct the VFS about our search paths
	initialiseVfs();
}

void Manager::showGameSetupDialog()
{
	// Paths not valid, dispatch a message
	ConfigurationNeeded message;

	GlobalRadiantCore().getMessageBus().sendMessage(message);

	if (message.isHandled())
	{
		applyConfig(message.getConfig());
	}
	else
	{
		throw std::runtime_error(_("No valid game configuration found, cannot continue."));
	}
}

void Manager::setMapAndPrefabPaths(const std::string& baseGamePath)
{
   // Construct the map path and make sure the folder exists
   // Get the maps folder (e.g. "maps/")
   std::string mapFolder = currentGame()->getLocalXPath(GKEY_MAPS_FOLDER)[0].getAttributeValue("value");
   if (mapFolder.empty())
   {
      mapFolder = "maps/";
   }

   if (_config.modPath.empty() && _config.modBasePath.empty())
   {
	   _mapPath = baseGamePath + mapFolder;
   }
   else if (!_config.modPath.empty())
   {
	   _mapPath = _config.modPath + mapFolder;
   }
   else // _config.modBasePath is not empty
   {
	   _mapPath = _config.modBasePath + mapFolder;
   }

   rMessage() << "GameManager: Map path set to " << _mapPath << std::endl;
   os::makeDirectory(_mapPath);

   // Setup the prefab path
   _prefabPath = _mapPath;
   std::string pfbFolder = currentGame()->getLocalXPath(GKEY_PREFAB_FOLDER)[0].getAttributeValue("value");

   // Replace the "maps/" with "prefabs/"
   string::replace_last(_prefabPath, mapFolder, pfbFolder);
   // Store the path into the registry
   rMessage() << "GameManager: Prefab path set to " << _prefabPath << std::endl;
}

void Manager::initialiseVfs()
{
	// Ensure that all paths are normalised
	GameConfigUtil::EnsurePathsNormalised(_config);

	// The list of paths which will be passed to the VFS init method
	vfs::SearchPaths vfsSearchPaths;

	std::set<std::string> extensions;
	string::split(extensions, currentGame()->getKeyValue("archivetypes"), " ");

    for (const auto& extension : extensions)
    {
        auto extLower = string::to_lower_copy(extension);
        GlobalFiletypes().registerPattern(filetype::TYPE_PAK,
            FileTypePattern(fmt::format(_("{0} File"), string::to_upper_copy(extension)), extLower, "*." + extLower, PAK_ICON));
    }

	if (!_config.modPath.empty())
	{
		// We have a MOD, register this directory first
		vfsSearchPaths.insertIfNotExists(_config.modPath);

#ifdef POSIX
		std::string fsGame = os::getRelativePath(_config.modPath, _config.enginePath);

		// On Linux, the above was in ~/.doom3/, search the engine mod path as well
		std::string baseModPath = os::standardPathWithSlash(_config.enginePath + fsGame);
		vfsSearchPaths.insertIfNotExists(baseModPath);
#endif
	}

	if (!_config.modBasePath.empty())
	{
		// We have a MOD base, register this directory as second
		vfsSearchPaths.insertIfNotExists(_config.modBasePath);

#ifdef POSIX
		std::string fsGameBase = os::getRelativePath(_config.modBasePath, _config.enginePath);

		// On Linux, the above was in ~/.doom3/, search the engine mod base path as well
		std::string baseModPath = os::standardPathWithSlash(_config.enginePath + fsGameBase);
		vfsSearchPaths.insertIfNotExists(baseModPath);
#endif
	}

	// On UNIX this is the user-local enginepath, e.g. ~/.doom3/base/
	// On Windows this will be the same as global engine path
	std::string userBasePath = os::standardPathWithSlash(
		getUserEnginePath() + // ~/.doom3
		currentGame()->getKeyValue("basegame") // base
	);
	vfsSearchPaths.insertIfNotExists(userBasePath);

	// Register the base game folder (/usr/local/games/doom3/<basegame>) last
	// This will always be searched, but *after* the other paths
	std::string baseGame = os::standardPathWithSlash(
		_config.enginePath + currentGame()->getKeyValue("basegame")
	);
	vfsSearchPaths.insertIfNotExists(baseGame);

	// Update map and prefab paths
	setMapAndPrefabPaths(userBasePath);

	// Initialise the filesystem, if we were initialised before
	GlobalFileSystem().initialise(vfsSearchPaths, extensions);
}

const Manager::PathList& Manager::getVFSSearchPaths() const
{
	return GlobalFileSystem().getVfsSearchPaths();
}

const std::string& Manager::getMapPath()
{
	return _mapPath;
}

const std::string& Manager::getPrefabPath()
{
	return _prefabPath;
}

const std::string& Manager::getEnginePath() const
{
	return _config.enginePath;
}

void Manager::loadGameFiles(const std::string& appPath)
{
	std::string gamePath = appPath + "games/";
	rMessage() << "GameManager: Scanning for game description files: " << gamePath << std::endl;

    try
    {
		// Invoke a functor on every file in the games/ dir,
		// function gets called with the file (without path)
		os::foreachItemInDirectory(gamePath, [&](const fs::path& file)
		{
			if (string::to_lower_copy(file.extension().string()) != Game::FILE_EXTENSION)
			{
				// Don't process files not ending with .game
				return;
			}

			// Create a new Game object
			GamePtr newGame = std::make_shared<Game>(gamePath, file.filename().string());
			std::string gameName = newGame->getName();

			if (!gameName.empty())
			{
				// Store the game into the map
				_games[gameName] = newGame;
			}
		});

        rMessage() << "GameManager: Found game definitions: " << std::endl;

        for (GameMap::value_type& pair : _games)
        {
            rMessage() << "  " << pair.first << std::endl;
        }

		// Populate the sorted games list
		_sortedGames.clear();

		// Sort the games by their index
		std::multimap<int, GamePtr> sortedGameMap;

		for (const GameMap::value_type& pair : _games)
		{
			int index = string::convert<int>(pair.second->getKeyValue("index"), 9999);
			sortedGameMap.insert(std::make_pair(index, pair.second));
		}

		for (const auto& pair : sortedGameMap)
		{
			_sortedGames.push_back(pair.second);
		}

        rMessage() << std::endl;
    }
    catch (os::DirectoryNotFoundException&)
    {
        rError() << "Could not find directory with game files in " << gamePath << std::endl;
    }
}

// The static module definition (self-registers)
module::StaticModuleRegistration<Manager> gameManagerModule;

} // namespace game
