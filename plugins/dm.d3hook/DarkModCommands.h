#ifndef DARKMODCOMMANDS_H_
#define DARKMODCOMMANDS_H_

#include <string>

/**
 * greebo: A generic container providing various static darkmod commands. 
 */
class DarkModCommands {
public:
	// Compiles the map (including or excluding AAS)
	static void compileMap(bool noAAS = false);
	
	// Command targets for the EventManager
	static void compileMap();
	static void compileMapNoAAS();
	static void runAAS();
	
private:
	// Tries to locate the name of the current map
	static std::string getMapName();
};

#endif /*DARKMODCOMMANDS_H_*/
