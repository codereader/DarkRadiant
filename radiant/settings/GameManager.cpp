#include "GameManager.h"

#include "i18n.h"
#include "iregistry.h"
#include "itextstream.h"
#include "ifiletypes.h"
#include "ifilesystem.h"
#include "settings/PreferenceSystem.h"
#include "ui/prefdialog/PrefDialog.h"

#include "os/file.h"
#include "os/dir.h"
#include "os/path.h"
#include "os/fs.h"

#include "GameFileLoader.h"
#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/dialog/MessageBox.h"
#include "Win32Registry.h"
#include "modulesystem/StaticModule.h"
#include "modulesystem/ApplicationContextImpl.h"

#include <iostream>

using boost::format;

namespace game
{
	namespace {
		const std::string RKEY_GAME_TYPE = "user/game/type";
		const std::string RKEY_FS_GAME = "user/game/fs_game";
		const std::string RKEY_FS_GAME_BASE = "user/game/fs_game_base";
		const char* const GKEY_PREFAB_FOLDER = "/mapFormat/prefabFolder";
		const char* const GKEY_MAPS_FOLDER = "/mapFormat/mapFolder";
	}

Manager::Manager() :
	_enginePathInitialised(false)
{}

const std::string& Manager::getName() const {
	static std::string _name(MODULE_GAMEMANAGER);
	return _name;
}

const StringSet& Manager::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_FILETYPES);
	}

	return _dependencies;
}

void Manager::initialiseModule(const ApplicationContext& ctx)
{
	initialise(ctx.getRuntimeDataPath());

	initEnginePath();
}

const std::string& Manager::getFSGame() const {
	return _fsGame;
}

const std::string& Manager::getFSGameBase() const {
	return _fsGameBase;
}

const std::string& Manager::getModPath() const {
	// Return the fs_game path if available
	return (!_modPath.empty()) ? _modPath : _modBasePath;
}

const std::string& Manager::getModBasePath() const
{
	return _modBasePath;
}

IGamePtr Manager::currentGame()
{
	if (_currentGameName.empty())
	{
		// No game type selected, bail out, the program will crash anyway on module load
		gtkutil::MessageBox::ShowFatalError(_("GameManager: No game type selected, can't continue."), Glib::RefPtr<Gtk::Window>());
	}

	return _games[_currentGameName];
}

void Manager::constructPreferences()
{
	PreferencesPagePtr page = GetPreferenceSystem().getPage(_("Game"));

	ComboBoxValueList gameList;
	for (GameMap::iterator i = _games.begin(); i != _games.end(); ++i)
	{
		gameList.push_back(i->second->getKeyValue("name"));
	}
	page->appendCombo(_("Select a Game:"), RKEY_GAME_TYPE, gameList, true);
	page->appendPathEntry(_("Engine Path"), RKEY_ENGINE_PATH, true);
	page->appendEntry(_("Mod (fs_game)"), RKEY_FS_GAME);
	page->appendEntry(_("Mod Base (fs_game_base, optional)"), RKEY_FS_GAME_BASE);
}

void Manager::initialise(const std::string& appPath)
{
	// Scan the <applicationpath>/games folder for .game files
	loadGameFiles(appPath);
   if (_games.empty())
   {
      // No game types available, bail out, the program would crash anyway on
      // module load
      gtkutil::MessageBox::ShowFatalError(
		  _("GameManager: No valid game files found, can't continue."), Glib::RefPtr<Gtk::Window>()
      );
   }

	// Add the settings widgets to the Preference Dialog, we might need it
	constructPreferences();

	// Find the user's selected game from the XML registry, and attempt to find
   // it in the set of available games.
	std::string gameType = GlobalRegistry().get(RKEY_GAME_TYPE);
	GameMap::iterator i = _games.find(gameType);

	if (gameType.empty() || i == _games.end())
   {
		// We have at least one game type available, select the first
		// Store the name of the only game into the Registry
		GlobalRegistry().set(RKEY_GAME_TYPE, _games.begin()->first);
	}

	// Load the value from the registry, there should be one selected at this point
	_currentGameName = GlobalRegistry().get(RKEY_GAME_TYPE);

	// The game type should be selected now
	if (!_currentGameName.empty())
    {
		rMessage() << "GameManager: Selected game type: "
                             << _currentGameName << std::endl;
	}
	else {
		// No game type selected, bail out, the program would crash anyway on module load
		gtkutil::MessageBox::ShowFatalError(_("No game type selected."), Glib::RefPtr<Gtk::Window>());
	}
}

std::string Manager::getUserEnginePath()
{
#if defined(POSIX)

    // First check for a local copy of the game tree, e.g. ~/.doom3
    fs::path localPath = fs::path(Glib::get_home_dir())
                         / currentGame()->getKeyValue("prefix");

    if (fs::exists(localPath))
    {
        return os::standardPathWithSlash(localPath);
    }

#endif

    // Otherwise (Windows, or no local mod path found) return the regular engine
    // path
    return _enginePath;
}

void Manager::constructPaths()
{
	_enginePath = GlobalRegistry().get(RKEY_ENGINE_PATH);

	// Make sure it's a well formatted path
	_enginePath = os::standardPathWithSlash(_enginePath);

	// Read command line parameters, these override any existing preference setting
    const ApplicationContext::ArgumentList& args(
        module::getRegistry().getApplicationContext().getCmdLineArgs()
    );

    for (ApplicationContext::ArgumentList::const_iterator i = args.begin();
         i != args.end();
         ++i)
    {
		// get the argument and investigate it
		std::string arg = *i;

		if (boost::algorithm::istarts_with(arg, "fs_game=")) {
			GlobalRegistry().set(RKEY_FS_GAME, arg.substr(8));
		}
		else if (boost::algorithm::istarts_with(arg, "fs_game_base=")) {
			GlobalRegistry().set(RKEY_FS_GAME_BASE, arg.substr(13));
		}
	}

	// Load the fsGame and fsGameBase from the registry
	_fsGame = GlobalRegistry().get(RKEY_FS_GAME);
	_fsGameBase = GlobalRegistry().get(RKEY_FS_GAME_BASE);

	if (!_fsGameBase.empty())
	{
		// greebo: #3480 check if the mod base path is absolute. If not, append it to the engine path
		_modBasePath = fs::path(_fsGameBase).is_absolute() ? _fsGameBase : getUserEnginePath() + _fsGameBase;

		// Normalise the path as last step
		_modBasePath = os::standardPathWithSlash(_modBasePath);
	}
	else
	{
		// No fs_game_base, no mod base path
		_modBasePath = "";
	}

	if (!_fsGame.empty())
	{
		// greebo: #3480 check if the mod path is absolute. If not, append it to the engine path
		_modPath = fs::path(_fsGame).is_absolute() ? _fsGame : getUserEnginePath() + _fsGame;

		// Normalise the path as last step
		_modPath = os::standardPathWithSlash(_modPath);
	}
	else
	{
		// No fs_game, no modpath
		_modPath = "";
	}
}

bool Manager::userWantsToCorrectSettings() const
{
    using boost::format;
    std::stringstream msg("<b>Warning:</b>\n");

    if (!os::fileOrDirExists(_enginePath))
    {
        msg << format(_("Engine path \"%1%\" does not exist.\n")) % _enginePath;
    }

    if (!_fsGame.empty() && !os::fileOrDirExists(_modPath))
    {
        msg << format(_("The fs_game folder \"%1%\" does not exist.\n"))
               % _modPath;
    }

    if (!_fsGameBase.empty() && !os::fileOrDirExists(_modBasePath))
    {
        msg << format(_("The fs_game_base folder \"%1%\" does not exist.\n"))
               % _modBasePath;
    }

    msg << _("Do you want to correct these settings?");

    gtkutil::MessageBox msgBox(
        _("Invalid Settings"), msg.str(), ui::IDialog::MESSAGE_ASK
    );
    return (msgBox.run() == ui::IDialog::RESULT_YES);
}

void Manager::initEnginePath()
{
	// Try to retrieve a saved value for the engine path
	std::string enginePath = GlobalRegistry().get(RKEY_ENGINE_PATH);
	xml::NodeList gameNodeList = GlobalRegistry().findXPath("game");

	if (enginePath.empty() && gameNodeList.size() > 0) {
		// No engine path known, but we have a valid game description
		// Try to deduce the engine path from the Registry settings (Win32 only)
		std::string regKey = gameNodeList[0].getAttributeValue("registryKey");
		std::string regValue = gameNodeList[0].getAttributeValue("registryValue");

		rMessage() << "GameManager: Querying Windows Registry for game path: "
							 << "HKEY_LOCAL_MACHINE\\"
							 << regKey << "\\" << regValue << std::endl;

		// Query the Windows Registry for a default installation path
		// This will return "" for non-Windows environments
		enginePath = Win32Registry::getKeyValue(regKey, regValue);

		rMessage() << "GameManager: Windows Registry returned result: "
							 << enginePath << std::endl;
	}

	// If the engine path is still empty, consult the .game file for a fallback value
	if (enginePath.empty()) {
		// No engine path set so far, search the game file for default values
		const std::string ENGINEPATH_ATTRIBUTE =
#if defined(WIN32)
		    "enginepath_win32"
#elif defined(__linux__) || defined (__FreeBSD__)
		    "enginepath_linux"
#elif defined(__APPLE__)
		    "enginepath_macos"
#else
#error "unknown platform"
#endif
    	;

	    enginePath = os::standardPathWithSlash(
			currentGame()->getKeyValue(ENGINEPATH_ATTRIBUTE)
		);
	}

	// Normalise the path in any case
	enginePath = os::standardPathWithSlash(enginePath);

	// Take this path and store it into the Registry, it's expected to be there
	GlobalRegistry().set(RKEY_ENGINE_PATH, enginePath);

	// Try to do something with the information currently in the Registry
	// It should be enough to know the engine path and the fs_game.
	constructPaths();

	// Check loop, continue, till the user specifies a valid setting
	while (!settingsValid())
	{
		// Engine path doesn't exist, ask the user
		ui::PrefDialog::showModal(_("Game"));

		// After the dialog, the settings are located in the registry.
		// Construct the paths with the settings found there
		constructPaths();

		if (!settingsValid() && !userWantsToCorrectSettings())
        {
            break;
		}
	}

	// Register as observer, to get notified about future engine path changes
	observeKey(RKEY_ENGINE_PATH);
	observeKey(RKEY_FS_GAME);
	observeKey(RKEY_FS_GAME_BASE);

	// Force an update ((re-)initialises the VFS)
	updateEnginePath(true);

	// Add the note to the preference page
	PreferencesPagePtr page = GetPreferenceSystem().getPage(_("Game"));
	page->appendLabel(_("<b>Note</b>: You will have to restart DarkRadiant for the changes to take effect."));
}

void Manager::observeKey(const std::string& key)
{
    // Hide std::string signal argument, replace with bound false value for
    // updateEnginePath().
	GlobalRegistry().signalForKey(key).connect(
        sigc::bind(sigc::mem_fun(this, &Manager::updateEnginePath), false)
    );
}

void Manager::addVFSSearchPath(const std::string &path)
{
	// If this path is searched earlier then the file would be found the first time.
	for (PathList::iterator i = _vfsSearchPaths.begin(); 
		 i != _vfsSearchPaths.end(); ++i)
	{
		if (*i == path)
		{
			return;
		}
	}

	_vfsSearchPaths.push_back(path);
}

bool Manager::settingsValid() const
{
	if (os::fileOrDirExists(_enginePath)) {

		// Check the mod base path, if appropriate
		if (!_fsGameBase.empty() && !os::fileOrDirExists(_modBasePath)) {
			// Mod base name is not empty, but folder doesnt' exist
			return false;
		}

		// Check the mod path, if appropriate
		if (!_fsGame.empty() && !os::fileOrDirExists(_modPath)) {
			// Mod name is not empty, but mod folder doesnt' exist
			return false;
		}

		return true; // all settings there
	}

	// Engine path doesn't exist
	return false;
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

   if (_fsGame.empty() && _fsGameBase.empty()) {
      mapPath = baseGamePath + mapFolder;
   }
   else if (!_fsGame.empty()) {
      mapPath = _modPath + mapFolder;
   }
   else { // fsGameBase is not empty
      mapPath = _modBasePath + mapFolder;
   }
   rMessage() << "GameManager: Map path set to " << mapPath << std::endl;
   os::makeDirectory(mapPath);

   // Save the map path to the registry
   GlobalRegistry().set(RKEY_MAP_PATH, mapPath);

   // Setup the prefab path
   std::string prefabPath = mapPath;
   std::string pfbFolder = currentGame()->getLocalXPath(GKEY_PREFAB_FOLDER)[0].getAttributeValue("value");

   // Replace the "maps/" with "prefabs/"
   boost::algorithm::replace_last(prefabPath, mapFolder, pfbFolder);
   // Store the path into the registry
   rMessage() << "GameManager: Prefab path set to " << prefabPath << std::endl;
   GlobalRegistry().set(RKEY_PREFAB_PATH, prefabPath);
}

void Manager::updateEnginePath(bool forced)
{
	// Clean the new path
	std::string newPath = os::standardPathWithSlash(
		GlobalRegistry().get(RKEY_ENGINE_PATH)
	);

	std::string newFSGame = GlobalRegistry().get(RKEY_FS_GAME);
	std::string newFSGameBase = GlobalRegistry().get(RKEY_FS_GAME_BASE);

	// Only update if any settings were changed, or if this is a "forced" update
	if (newPath != _enginePath || newFSGame != _fsGame || newFSGameBase != _fsGameBase || forced)
	{
		bool enginePathWasInitialised = _enginePathInitialised;

		// Check, if the engine path was already initialised in this session
		// If yes, shutdown the virtual filesystem.
		if (enginePathWasInitialised)
		{
			GlobalFileSystem().shutdown();
			GlobalRegistry().set(RKEY_MAP_PATH, "");
			_enginePathInitialised = false;
			_vfsSearchPaths.clear();
		}

		// Set the new fs_game and engine paths
		_fsGame = newFSGame;
		_fsGameBase = newFSGameBase;
		_enginePath = newPath;

		// Reconstruct the paths basing on these two, the _modBasePath may be out of date
		constructPaths();

		if (!_enginePathInitialised)
		{
			if (!_fsGame.empty()) {
				// We have a MOD, register this directory first
				addVFSSearchPath(_modPath);

#if defined(POSIX)
				// On Linux, the above was in ~/.doom3/, search the engine mod path as well
				std::string baseModPath = os::standardPathWithSlash(_enginePath + _fsGame);
				addVFSSearchPath(baseModPath);
#endif
			}

			if (!_fsGameBase.empty()) {
				// We have a MOD base, register this directory as second
				addVFSSearchPath(_modBasePath);

#if defined(POSIX)
				// On Linux, the above was in ~/.doom3/, search the engine mod path as well
				std::string baseModPath = os::standardPathWithSlash(_enginePath + _fsGameBase);
				addVFSSearchPath(baseModPath);
#endif
			}

			// On UNIX this is the user-local enginepath, e.g. ~/.doom3/base/
			// On Windows this will be the same as global engine path
			std::string userBasePath = os::standardPathWithSlash(
				getUserEnginePath() + // ~/.doom3
				currentGame()->getKeyValue("basegame") // base
			);
			addVFSSearchPath(userBasePath);

			// Register the base game folder (/usr/local/games/doom3/<basegame>) last
			// This will always be searched, but *after* the other paths
			std::string baseGame = os::standardPathWithSlash(
				_enginePath + currentGame()->getKeyValue("basegame")
			);
			addVFSSearchPath(baseGame);

			// Update map and prefab paths
			setMapAndPrefabPaths(userBasePath);

			_enginePathInitialised = true;

			rMessage() << "VFS Search Path priority is: " << std::endl;
			for (PathList::iterator i = _vfsSearchPaths.begin();
				i != _vfsSearchPaths.end(); ++i)
			{
				rMessage() << "- " << (*i) << std::endl;
			}

			if (enginePathWasInitialised)
			{
				// Re-initialise the filesystem, if we were initialised before
				GlobalFileSystem().initialise();
			}
		}
	}
}

const Manager::PathList& Manager::getVFSSearchPaths() const {
	// Should not be called before the list is initialised
	if (_vfsSearchPaths.empty()) {
		std::cout << "GameManager: Warning, VFS search paths not yet initialised.";
	}
	return _vfsSearchPaths;
}

const std::string& Manager::getEnginePath() const {
	return _enginePath;
}

void Manager::loadGameFiles(const std::string& appPath)
{
	std::string gamePath = appPath + "games/";
	rMessage() << "GameManager: Scanning for game description files: " << gamePath << std::endl;

	// Invoke a GameFileLoader functor on every file in the games/ dir.
	GameFileLoader gameFileLoader(_games, gamePath);
	Directory_forEach(gamePath.c_str(), gameFileLoader);

	rMessage() << "GameManager: Found game definitions: " << std::endl;
	for (GameMap::iterator i = _games.begin(); i != _games.end(); ++i)
	{
		rMessage() << "  " << i->first << std::endl;
	}
	rMessage() << std::endl;
}

} // namespace game

// The static module definition (self-registers)
module::StaticModule<game::Manager> gameManagerModule;
