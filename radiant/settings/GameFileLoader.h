#ifndef GAMEFILELOADER_H_
#define GAMEFILELOADER_H_

#include "GameManager.h"

namespace game {

class GameFileLoader
{
	// Reference to external map of Game objects 
	Manager::GameMap& _games;

	// Path to the game file to be added
	const std::string _path;

public:
	// Constructor
	GameFileLoader(Manager::GameMap& games, const std::string& path);

	// Main functor () function, gets called with the file (without path)
	void operator() (const std::string& name);
};

} // namespace game

#endif /*GAMEFILELOADER_H_*/
