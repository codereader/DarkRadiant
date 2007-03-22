#include "GameManager.h"

#include "iregistry.h"
#include "ifilesystem.h"
#include "preferences.h"
#include "environment.h"
#include "os/file.h"
#include "os/dir.h"
#include "os/path.h"
#include "cmdlib.h"
#include "GameFileLoader.h"
#include "gtkutil/dialog.h"
#include "gtkutil/messagebox.h"
#include "mainframe.h"
#include <gtk/gtkmain.h>

namespace game {

	namespace {
		const std::string RKEY_GAME_TYPE = "user/game/type";
		const std::string RKEY_FS_GAME = "user/game/fs_game";
	}

Manager::Manager() :
	_enginePathInitialised(false)
{}

std::string Manager::getFSGame() const {
	return _fsGame;
}

/** greebo: Returns the current Game.
 */
GamePtr Manager::currentGame() {
	if (_currentGameType.empty()) {
		// No game type selected, bail out, the program will crash anyway on module load
		gtkutil::fatalErrorDialog("GameManager: No game type selected, can't continue.\n", NULL);
	}
	
	return _games[_currentGameType];
}

void Manager::constructPreferences() {
	PreferencesPagePtr page = GetPreferenceSystem().getPage("Game");
	page->appendPathEntry("Engine Path", RKEY_ENGINE_PATH, true);
	page->appendEntry("Game Mod (fs_game)", RKEY_FS_GAME);
}

/** greebo: Loads the game files and the saved settings.
 * 			If no saved game setting is found, the user
 * 			is asked to enter the relevant information in a Dialog. 
 */
void Manager::initialise() {
	// Add the settings widgets to the Preference Dialog, we might need it
	constructPreferences();
	
	// Scan the <applicationpath>/games folder for .game files
	loadGameFiles();
	
	if (GlobalRegistry().get(RKEY_GAME_TYPE).empty()) {
		// Check the number of available games
		if (_games.size() == 0) {
			// No game type selected, bail out, the program will crash anyway on module load
			gtkutil::fatalErrorDialog("GameManager: No valid game files found, can't continue\n", NULL);
		}
		else if (_games.size() == 1) {
			// We have exacty one game type available, auto-select it
			// Store the name of the only game into the Registry 
			GlobalRegistry().set(RKEY_GAME_TYPE, _games.begin()->first);
		}
		else {
			// More than one game available, show the dialog
		}
	}
	
	// Load the value from the registry, there should be one selected at this point
	_currentGameType = GlobalRegistry().get(RKEY_GAME_TYPE);
	
	// The game type should be selected now
	if (!_currentGameType.empty()) {
		globalOutputStream() << "GameManager: Selected game type: " << _currentGameType.c_str() << "\n";
	}
	else {
		// No game type selected, bail out, the program will crash anyway on module load
		gtkutil::fatalErrorDialog("No game type selected\n", NULL);
	}
}

std::string Manager::getUserEnginePath() {
#if defined(POSIX)
		// Get the user home folder
		std::string homeDir = os::standardPathWithSlash(g_get_home_dir());
		// Get the game prefix, which has to be appended (e.g. ".doom3")
		std::string prefix = currentGame()->getKeyValue("prefix");
		
		// Construct the user's engine path
		return os::standardPathWithSlash(homeDir + prefix + "/");
#else
		// In all other environments, we just take the engine path as base
		return _enginePath;
#endif
}

void Manager::constructPaths() {
	_enginePath = GlobalRegistry().get(RKEY_ENGINE_PATH);
	
	// Make sure it's a well formatted path
	_enginePath = os::standardPathWithSlash(_enginePath);
	
	// Load the fsGame from the registry
	_fsGame = GlobalRegistry().get(RKEY_FS_GAME);
	
	if (!_fsGame.empty()) {	
		// Create the mod path
		_modPath = os::standardPathWithSlash(getUserEnginePath() + _fsGame);
	}
	else {
		// No fs_game, no modpath
		_modPath = "";
	}
}

void Manager::initEnginePath() {
	// Try to retrieve a saved value for the engine path
	std::string enginePath = GlobalRegistry().get(RKEY_ENGINE_PATH);
	
	if (enginePath.empty()) {
		// No engine path set so far, search the game file for default values
		const char* ENGINEPATH_ATTRIBUTE =
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
			currentGame()->getRequiredKeyValue(ENGINEPATH_ATTRIBUTE)
		);
	}
	
	// Take this path and store it into the Registry, it's expected to be there
	GlobalRegistry().set(RKEY_ENGINE_PATH, enginePath);
	
	// Try to do something with the information currently in the Registry
	// It should be enough to know the engine path and the fs_game.
	constructPaths();
	
	// Check loop, continue, till the user specifies a valid setting
	while (!settingsValid()) {
		// Engine path doesn't exist, ask the user
		PrefsDlg::showModal("Game");
		
		// After the dialog, the settings are located in the registry.
		// Construct the paths with the settings found there		
		constructPaths();
		
		if (!settingsValid()) {
			std::string msg("<b>Warning:</b>\n");
			
			if (!file_exists(_enginePath.c_str())) {
				msg += "Engine path does not exist.\n";
			}
			
			if (!_fsGame.empty() && !file_exists(_modPath.c_str())) {
				msg += "The fs_game folder does not exist.\n";
			}
			msg += "Do you want to correct these settings?";
			if (gtk_MessageBox(0, msg.c_str(), "Invalid Settings", eMB_YESNO, eMB_ICONQUESTION) == eIDNO) {
				break;
			}
		}
	}
	
	// Register as observer, to get notified about future engine path changes
	GlobalRegistry().addKeyObserver(this, RKEY_ENGINE_PATH);
	GlobalRegistry().addKeyObserver(this, RKEY_FS_GAME);
	
	// Force an update ((re-)initialises the VFS)
	updateEnginePath(true);
	
	// Add the note to the preference page
	PreferencesPagePtr page = GetPreferenceSystem().getPage("Game");
	page->appendLabel("<b>Note</b>: You will have to restart DarkRadiant for the changes to take effect.");
}

bool Manager::settingsValid() const {
	if (file_exists(_enginePath.c_str())) {
		// Return true, if the fs_game is empty OR the modPath exists
		return (_fsGame.empty() || file_exists(_modPath.c_str())); 
	}
	return false;
}

void Manager::keyChanged() {
	// call the engine path setter, fs_game is updated there as well
	updateEnginePath();
}

void Manager::setFSGame(const std::string& fsGame) {
	_fsGame = fsGame;
}

void Manager::updateEnginePath(bool forced) {
	// Clean the new path
	std::string newPath = os::standardPathWithSlash(
		GlobalRegistry().get(RKEY_ENGINE_PATH)
	);
	std::string newFSGame = GlobalRegistry().get(RKEY_FS_GAME);
	
	if (newPath != _enginePath || newFSGame != _fsGame || forced) {
		// Set the new fs_game
		_fsGame = newFSGame;
		
		if (_enginePathInitialised) {
			GlobalFileSystem().shutdown();
			Environment::Instance().setMapsPath("");
			_enginePathInitialised = false;
		}
		
		_enginePath = newPath;
		
		if (!_enginePathInitialised) {
			//EnginePath_Realise();
			if (!_fsGame.empty()) {
				// We have a MOD, register this directory first
				GlobalFileSystem().initDirectory(_modPath);
				
#if defined(POSIX)
				// On Linux, the above was in ~/.doom3/, search the engine mod path as well
				std::string baseModPath = os::standardPathWithSlash(_enginePath + _fsGame);
				GlobalFileSystem().initDirectory(baseModPath);
#endif
			}
			
#if defined(POSIX)
			// this is the *NIX path ~/.doom3/base/
			std::string userBasePath = os::standardPathWithSlash(
				getUserEnginePath() + _enginePath + currentGame()->getRequiredKeyValue("basegame")
			);
			GlobalFileSystem().initDirectory(userBasePath);
#endif
			
			// Register the base game folder (/usr/local/games/doom3/<basegame>) last
			// This will always be searched, but *after* the other paths
			std::string baseGame = os::standardPathWithSlash(
				_enginePath + currentGame()->getRequiredKeyValue("basegame")
			);
			GlobalFileSystem().initDirectory(baseGame);
			
			// Construct the map path and make sure the folder exists
			std::string mapPath; 
			if (_fsGame.empty()) {
				mapPath = _enginePath + "maps/";
			}
			else {
				mapPath = _modPath + "maps/";
			}
			globalOutputStream() << "GameManager: Map path set to " << mapPath.c_str() << "\n";
			Q_mkdir(mapPath.c_str());
			// Save the map path (is stored to the registry as well)
			Environment::Instance().setMapsPath(mapPath);

			_enginePathInitialised = true;
		}
	}
}

std::string Manager::getEnginePath() const {
	return _enginePath;
}

const char* Manager::getCurrentGameType() {
	return _currentGameType.c_str();
}

/** greebo: Scans the "games/" subfolder for .game description foles.
 */
void Manager::loadGameFiles() {
	std::string gamePath = GlobalRegistry().get(RKEY_APP_PATH) + "games/";
	
	globalOutputStream() << "GameManager: Scanning for game description files: " << gamePath.c_str() << '\n';

	// Invoke a GameFileLoader functor on every file in the games/ dir.
	Directory_forEach(gamePath.c_str(), GameFileLoader(_games, gamePath.c_str()));
	
	globalOutputStream() << "GameManager: Found game definitions: ";
	for (GameMap::iterator i = _games.begin(); i != _games.end(); i++) {
		globalOutputStream() << i->first.c_str() << " ";
	}
	globalOutputStream() << "\n";
}

// Accessor method containing the static instance
Manager& Manager::Instance() {
	static Manager _instance;
	return _instance;
}

} // namespace game
