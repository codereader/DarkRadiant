#pragma once

#include "GameManager.h"
#include "os/fs.h"

namespace game
{

class GameFileLoader
{
	// Reference to external map of Game objects
	Manager::GameMap& _games;

	// Path to the game file to be added
	const std::string _path;

public:
	// Constructor
	GameFileLoader(Manager::GameMap& games, const std::string& path);

	// Main functor () function, gets called with the file
	void operator() (const fs::path& file);
};

} // namespace game

