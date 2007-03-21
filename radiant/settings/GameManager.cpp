#include "GameManager.h"

#include "iregistry.h"
#include "preferences.h"
#include "environment.h"
#include "os/file.h"
#include "os/dir.h"
#include "os/path.h"
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
			game::Manager::Instance().currentGame()->getRequiredKeyValue(ENGINEPATH_ATTRIBUTE)
		);
	}
	
	// Take this path and check it
	GlobalRegistry().set(RKEY_ENGINE_PATH, enginePath);
	_enginePath = enginePath;
	
	// Load the fsGame from the registry
	_fsGame = GlobalRegistry().get(RKEY_FS_GAME);
	
	// Check loop, continue, till the user specifies a valid setting
	while (!settingsValid()) {
		// Engine path doesn't exist, ask the user
		PrefsDlg::showModal("Game");
		
		_enginePath = os::standardPathWithSlash(GlobalRegistry().get(RKEY_ENGINE_PATH));
		_fsGame = GlobalRegistry().get(RKEY_FS_GAME);
		
		if (!settingsValid()) {
			std::string msg("<b>Warning:</b>\n");
			
			if (!file_exists(_enginePath.c_str())) {
				msg += "Engine path does not exist.\n";
			}
			
			std::string fsGamePath = os::standardPathWithSlash(_enginePath + _fsGame);
			if (!_fsGame.empty() && !file_exists(fsGamePath.c_str())) {
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
	
	// Trigger a VFS Update
	setEnginePath(_enginePath);
}

bool Manager::settingsValid() const {
	if (file_exists(_enginePath.c_str())) {
		// Do we have a custom game mod?
		if (_fsGame.empty()) {
			// No, everything is ok
			return true;
		}
		else {
			std::string fsGamePath = os::standardPathWithSlash(_enginePath + _fsGame);
			return file_exists(fsGamePath.c_str());
		} 
	}
	return false;
}

void Manager::keyChanged() {
	setEnginePath(GlobalRegistry().get(RKEY_ENGINE_PATH));
	setFSGame(GlobalRegistry().get(RKEY_FS_GAME));
}

void Manager::setFSGame(const std::string& fsGame) {
	_fsGame = fsGame;
}

void Manager::setEnginePath(const std::string& path) {
	// Clean the new path
	std::string newPath = os::standardPathWithSlash(path);
	
	if (newPath != _enginePath) {
		// Disable screen updates
		if (MainFrame_getWindow() != NULL) {
			ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Changing Engine Path");
		}
		
		if (_enginePathInitialised) {
			EnginePath_Unrealise();
			_enginePathInitialised = false;
		}
		
		_enginePath = newPath;
		
		if (!_enginePathInitialised) {
			EnginePath_Realise();
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
