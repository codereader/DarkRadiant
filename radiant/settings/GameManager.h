#ifndef GAMEMANAGER_H_
#define GAMEMANAGER_H_

#include <string>
#include <map>

#include "Game.h"

namespace game {

/** greebo: The Manager class for keeping track
 * 			of the possible games and the current game.
 */
class Manager
{
public:
	// The map containing the named Game objects 
	typedef std::map<std::string, GamePtr> GameMap;

private:	
	GameMap _games;
	
	std::string _currentGameType;
	
public:
	/** greebo: Returns the current Game (shared_ptr).
	 */
	GamePtr currentGame();
	
	/** greebo: Returns the type of the currently active game.
	 * 			This is a convenience method to be used when loading
	 * 			modules that require a game type like "doom3".
	 */
	const char* getCurrentGameType();
	
	/** greebo: Loads the game files and the saved settings.
	 * 			If no saved game setting is found, the user
	 * 			is asked to enter the relevant information in a Dialog. 
	 */
	void initialise();
	
	/** greebo: Scans the "games/" subfolder for .game description foles.
	 */
	void loadGameFiles();
	
	// Accessor method containing the static instance
	static Manager& Instance();
};

} // namespace game

#endif /*GAMEMANAGER_H_*/
