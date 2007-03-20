#include "GameManager.h"

#include "iregistry.h"
#include "environment.h"
#include "os/dir.h"
#include "GameFileLoader.h"

namespace game {

/** greebo: Returns the current Game.
 */
GamePtr Manager::currentGame() {
	if (_currentGameName.empty()) {
		// Show up the dialog for the user to select
	}
	
	return _games[_currentGameName];
}

/** greebo: Loads the game files and the saved settings.
 * 			If no saved game setting is found, the user
 * 			is asked to enter the relevant information in a Dialog. 
 */
void Manager::initialise() {
	// Scan the <applicationpath>/games folder for .game files
	loadGameFiles();
}

const char* Manager::getCurrentGameName() {
	return _currentGameName.c_str();
}

/** greebo: Scans the "games/" subfolder for .game description foles.
 */
void Manager::loadGameFiles() {
	std::string gamePath = GlobalRegistry().get(RKEY_APP_PATH) + "games/";
	
	globalOutputStream() << "Scanning for game description files: " << gamePath.c_str() << '\n';

	// Invoke a GameFileLoader functor on every file in the games/ dir.
	Directory_forEach(gamePath.c_str(), GameFileLoader(_games, gamePath.c_str()));
}

// Accessor method containing the static instance
Manager& Manager::Instance() {
	static Manager _instance;
	return _instance;
}

} // namespace game
