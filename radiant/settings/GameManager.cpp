#include "GameManager.h"

#include "i18n.h"
#include "iregistry.h"
#include "itextstream.h"
#include "ifiletypes.h"
#include "ifilesystem.h"
#include "settings/PreferenceSystem.h"
#include "ui/prefdialog/PrefDialog.h"
#include "ui/prefdialog/GameSetupDialog.h"

#include "os/file.h"
#include "os/dir.h"
#include "os/path.h"
#include "os/fs.h"
#include "string/trim.h"
#include "string/convert.h"
#include "string/replace.h"
#include "string/predicate.h"
#include "string/split.h"

#include "GameFileLoader.h"
#include "wxutil/dialog/MessageBox.h"
#include "modulesystem/StaticModule.h"
#include "modulesystem/ApplicationContextImpl.h"

#include <sigc++/bind.h>

#include <iostream>

namespace game
{

namespace
{
	const char* const GKEY_PREFAB_FOLDER = "/mapFormat/prefabFolder";
	const char* const GKEY_MAPS_FOLDER = "/mapFormat/mapFolder";
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
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void Manager::initialiseModule(const ApplicationContext& ctx)
{
	// Read command line parameters, these override any existing preference setting
	const ApplicationContext::ArgumentList& args = ctx.getCmdLineArgs();

	for (const std::string& arg : args)
	{
		if (string::istarts_with(arg, "fs_game="))
		{
			GlobalRegistry().set(RKEY_FS_GAME, arg.substr(8));
		}
		else if (string::istarts_with(arg, "fs_game_base="))
		{
			GlobalRegistry().set(RKEY_FS_GAME_BASE, arg.substr(13));
		}
	}

	GlobalCommandSystem().addCommand("ProjectSettings", 
		std::bind(&Manager::showGameSetupDialog, this, std::placeholders::_1));

	// Scan the <applicationpath>/games folder for .game files
	loadGameFiles(ctx.getRuntimeDataPath());

	// Check if there's a saved game type in the registry,
	// try to setup a default if nothing is found.
	initialiseGameType();

	// Check validity of the saved game configuration 
	// and invoke the UI if it's not a valid one.
	initialiseConfig();
}

const std::string& Manager::getFSGame() const
{
	return GlobalRegistry().get(RKEY_FS_GAME);
}

const std::string& Manager::getFSGameBase() const 
{
	return GlobalRegistry().get(RKEY_FS_GAME_BASE);
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
		wxutil::Messagebox::ShowFatalError(_("GameManager: No game type selected, can't continue."), NULL);
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
		// No game types available, bail out, the program would crash anyway on
		// module load
		wxutil::Messagebox::ShowFatalError(
			_("GameManager: No valid game files found, can't continue."), nullptr
		);
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
			wxutil::Messagebox::ShowFatalError(
				"GameManager: Sorted game list is empty, can't continue.", nullptr
			);
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
		wxutil::Messagebox::ShowFatalError(_("No game type selected."));
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

void Manager::initialiseConfig()
{
	// Try to retrieve a saved value for the game setup
	_config.loadFromRegistry();

	if (!_config.pathsValid())
	{
		showGameSetupDialog(cmd::ArgumentList());
	}

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

void Manager::showGameSetupDialog(const cmd::ArgumentList& args)
{
	// Paths not valid, ask the user to select something
	GameConfiguration result = ui::GameSetupDialog::Show(cmd::ArgumentList());

	if (!result.enginePath.empty())
	{
		_config = result;
		_config.saveToRegistry();
	}
}

void Manager::setMapAndPrefabPaths(const std::string& baseGamePath)
{
   // Construct the map path and make sure the folder exists
   std::string mapPath;

   // Get the maps folder (e.g. "maps/")
   std::string mapFolder = currentGame()->getLocalXPath(GKEY_MAPS_FOLDER)[0].getAttributeValue("value");
   if (mapFolder.empty()) {
      mapFolder = "maps/";
   }

   if (_config.modPath.empty() && _config.modBasePath.empty()) 
   {
      mapPath = baseGamePath + mapFolder;
   }
   else if (!_config.modPath.empty())
   {
      mapPath = _config.modPath + mapFolder;
   }
   else // _config.modBasePath is not empty
   { 
      mapPath = _config.modBasePath + mapFolder;
   }

   rMessage() << "GameManager: Map path set to " << mapPath << std::endl;
   os::makeDirectory(mapPath);

   // Save the map path to the registry
   registry::setValue(RKEY_MAP_PATH, mapPath);

   // Setup the prefab path
   std::string prefabPath = mapPath;
   std::string pfbFolder = currentGame()->getLocalXPath(GKEY_PREFAB_FOLDER)[0].getAttributeValue("value");

   // Replace the "maps/" with "prefabs/"
   string::replace_last(prefabPath, mapFolder, pfbFolder);
   // Store the path into the registry
   rMessage() << "GameManager: Prefab path set to " << prefabPath << std::endl;
   registry::setValue(RKEY_PREFAB_PATH, prefabPath);
}

void Manager::initialiseVfs()
{
	// Ensure that all paths are normalised
	_config.ensurePathsNormalised();

	// The list of paths which will be passed to the VFS init method
	vfs::SearchPaths vfsSearchPaths;

	vfs::VirtualFileSystem::ExtensionSet extensions;
	string::split(extensions, currentGame()->getKeyValue("archivetypes"), " ");

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

const std::string& Manager::getEnginePath() const
{
	return _config.enginePath;
}

void Manager::loadGameFiles(const std::string& appPath)
{
	std::string gamePath = appPath + "games/";
	rMessage() << "GameManager: Scanning for game description files: " << gamePath << std::endl;

	// Invoke a GameFileLoader functor on every file in the games/ dir.
	GameFileLoader gameFileLoader(_games, gamePath);
    
    try
    {
        os::foreachItemInDirectory(gamePath, gameFileLoader);

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

} // namespace game

// The static module definition (self-registers)
module::StaticModule<game::Manager> gameManagerModule;
