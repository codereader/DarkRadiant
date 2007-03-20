#ifndef GAMEFILELOADER_H_
#define GAMEFILELOADER_H_

#include <list>
#include "os/path.h"
class GameDescription;

// Functor object responsible for parsing and loading an individual .game
// file.

class GameFileLoader 
{
	// Reference to external list of valid game descriptions. See the
	// GameDescription decl for more info.
	std::list<GameDescription*>& mGames;

	// Path to the game file to be added
	const char *mPath;

public:
	// Constructor
	GameFileLoader(std::list<GameDescription*>& games, const char *path);

	// Main functor () function, gets called with the file (without path)
	void operator() (const char *name) const;
};

#endif /*GAMEFILELOADER_H_*/
