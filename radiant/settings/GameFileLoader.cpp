#include "GameFileLoader.h"

#include <list>
#include "xmlutil/Document.h"
#include "itextstream.h"
#include "iregistry.h"
#include <iostream>

#include "GameDescription.h"
#include <boost/algorithm/string/predicate.hpp>

// Constructor
GameFileLoader::GameFileLoader(std::list<GameDescription*>& games, const char *path) :
	mGames(games),
	mPath(path) 
{}

// Main functor () function
void GameFileLoader::operator() (const char *name) const {
	// Only operate on .game files
	if (!extension_equal(path_get_extension(name), "game")) {
		return;
	}

	// Debug output
	std::string strPath = mPath;
	strPath += name;
	globalOutputStream() << strPath.c_str() << '\n';

	// Handle the XML file via libxml2
	xmlDocPtr pDoc = xmlParseFile(strPath.c_str());

	if (pDoc) {
		// Import the game file into the registry
		//GlobalRegistry().import(strPath, "", Registry::treeUser);
		
		// Parse success, add to list
		std::string fileName = name;
		mGames.push_front(new GameDescription(xml::Document(pDoc), fileName));
	}
	else {
		// Error
		globalErrorStream() << "XML parser failed on '" << strPath.c_str() << "'\n";
	}
}

namespace game {

// Constructor
GameFileLoader::GameFileLoader(Manager::GameMap& games, const std::string& path) :
	_games(games),
	_path(path)
{}

// Main functor () function, gets called with the file (without path)
void GameFileLoader::operator() (const char *name) const {
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
