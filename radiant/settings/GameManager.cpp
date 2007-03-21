#include "GameManager.h"

#include "iregistry.h"
#include "environment.h"
#include "os/dir.h"
#include "os/path.h"
#include "stream/stringstream.h"
#include "GameFileLoader.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"

namespace game {

	namespace {
		const std::string RKEY_GAME_TYPE = "user/game/type";
	}

Manager::Manager() :
	_enginePathInitialised(false)
{}

/** greebo: Returns the current Game.
 */
GamePtr Manager::currentGame() {
	if (_currentGameType.empty()) {
		// No game type selected, bail out, the program will crash anyway on module load
		gtkutil::fatalErrorDialog("GameManager: No game type selected, can't continue.\n", NULL);
	}
	
	return _games[_currentGameType];
}

/** greebo: Loads the game files and the saved settings.
 * 			If no saved game setting is found, the user
 * 			is asked to enter the relevant information in a Dialog. 
 */
void Manager::initialise() {
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
	    
		StringOutputStream path(256);
		path << DirectoryCleaned(game::Manager::Instance().currentGame()->getRequiredKeyValue(ENGINEPATH_ATTRIBUTE));
		enginePath = path.c_str();
	}
	
	// Take this engine path
	//_enginePath = enginePath;
	GlobalRegistry().set(RKEY_ENGINE_PATH, enginePath);
	// Trigger a VFS Update
	setEnginePath(enginePath);
	
	GlobalRegistry().addKeyObserver(this, RKEY_ENGINE_PATH);
}

void Manager::keyChanged() {
	setEnginePath(GlobalRegistry().get(RKEY_ENGINE_PATH));
}

void Manager::setEnginePath(const std::string& path) {
	// Clean the new path 
	StringOutputStream buffer(256);
	buffer << DirectoryCleaned(path.c_str());
	
	std::string newPath = buffer.c_str();
	
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
