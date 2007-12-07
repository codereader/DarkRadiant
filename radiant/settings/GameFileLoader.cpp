#include "GameFileLoader.h"

#include <boost/algorithm/string/predicate.hpp>

namespace game {

// Constructor
GameFileLoader::GameFileLoader(Manager::GameMap& games, const std::string& path) :
	_games(games),
	_path(path)
{}

// Main functor () function, gets called with the file (without path)
void GameFileLoader::operator() (const std::string& name) {
	if (!boost::algorithm::ends_with(name, GAME_FILE_EXT)) {
		// Don't process files not ending with .game
		return;
	}
	
	// Create a new Game object
	GamePtr newGame(new Game(_path, name));
	std::string gameType = newGame->getType();
	
	if (!gameType.empty()) {
		// Store the game into the map
		_games[gameType] = newGame;
	}
}

} // namespace game
