#pragma once

#include <string>
#include <map>

#include "igame.h"
#include "imodule.h"
#include "iregistry.h"
#include "Game.h"
#include "GameConfiguration.h"

namespace game
{

/** 
* greebo: The Manager class for keeping track of all the available 
* game types and the current game and path setup.
*/
class Manager : 
	public IGameManager
{
public:
	// The map containing the named Game objects
	typedef std::map<std::string, GamePtr> GameMap;

private:

   // Map of named games
	GameMap _games;
	// Map of indexed games
	GameList _sortedGames;

	// The currently active game configuration
	GameConfiguration _config;

private:
	// Set the map and prefab file paths from the current game information
	void setMapAndPrefabPaths(const std::string& baseGamePath);

	// greebo: Loads the paths from the registry and constructs a few secondary paths.
	void initialiseConfig();

public:
	Manager();

	// greebo: Sets up the VFS paths and calls initialise()
	void initialiseVfs();

	/** greebo: Gets the engine path (e.g. /usr/local/doom3/).
	 */
	const std::string& getEnginePath() const;

	/** greebo: Get the user engine path (is OS-specific)
	*/
	std::string getUserEnginePath() override;

	/**
	 * greebo: Gets the mod path (e.g. ~/.doom3/gathers/).
	 * Returns the mod base path if the mod path itself is empty.
	 */
	const std::string& getModPath() const override;

	/**
	 * greebo: Returns the mod base path (e.g. ~/.doom3/darkmod/),
	 * can be an empty string if fs_game_base is not set.
	 */
	const std::string& getModBasePath() const override;

	/** greebo: Accessor method for the fs_game parameter
	 */
	const std::string& getFSGame() const override;

	/** greebo: Accessor method for the fs_game_base parameter
	 */
	const std::string& getFSGameBase() const override;

	void showGameSetupDialog(const cmd::ArgumentList& args);

	/** greebo: Returns the current Game (shared_ptr).
	 */
	virtual IGamePtr currentGame() override;

	// Get the list of available games, sorted by their index
	const GameList& getSortedGameList() override;

	/** 
	 * greebo: Loads the game type from the saved settings.
	 * Tries to fall back to a reasonable default. Afterwards, the
	 * _config.gameType member is properly filled in.
	 */
	void initialiseGameType();

	/** greebo: Scans the "games/" subfolder for .game description foles.
	 */
	void loadGameFiles(const std::string& appPath);

	// Returns the sorted game path list
	virtual const PathList& getVFSSearchPaths() const override;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;

};

} // namespace game
