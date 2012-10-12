#ifndef GAMEMANAGER_H_
#define GAMEMANAGER_H_

#include <string>
#include <map>

#include "igame.h"
#include "imodule.h"
#include "iregistry.h"
#include "Game.h"

namespace game
{

/** greebo: The Manager class for keeping track
 * 			of the possible games and the current game.
 */
class Manager : public IGameManager
{
public:
	// The map containing the named Game objects
	typedef std::map<std::string, GamePtr> GameMap;

private:

   // Map of named games
	GameMap _games;

    // The name of the current game, e.g. "Doom 3"
	std::string _currentGameName;

	// The fs_game argument (usually "darkmod")
	std::string _fsGame;

	// Optional, defines the "mod base", which the fs_game builds upon
	std::string _fsGameBase;

	// The current engine path
	std::string _enginePath;

	// The "userengine" path (where the fs_game is stored)
	// this is ~/.doom3/<fs_game> in linux, and <enginepath>/<fs_game> in Win32
	std::string _modBasePath;

	// The "mod mod" path (where the fs_game_base is stored)
	// this is ~/.doom3/<fs_game_base> in linux, and <enginepath>/<fs_game_base> in Win32
	std::string _modPath;

	// The sorted list of VFS search paths (valid after module initialisation)
	PathList _vfsSearchPaths;

	bool _enginePathInitialised;

private:
    void observeKey(const std::string& key);
    bool userWantsToCorrectSettings() const;

   // Set the map and prefab file paths from the current game information
   void setMapAndPrefabPaths(const std::string& baseGamePath);

	/** greebo: Returns TRUE if the engine path exists and
	 * 			the fs_game (if it is non-empty) exists as well.
	 */
	bool settingsValid() const;

	/** greebo: Get the user engine path (is OS-specific)
	 */
	std::string getUserEnginePath();

	/** 
	 * DerSaidin: Adds a path to the VFS search list, skipping any duplicates.
	 * Note that the order of search paths must be preserved.
	 */
	void addVFSSearchPath(const std::string &path);

	/** greebo: Builds the paths (game engine, user game engine) with
	 * 			respect to the OS we're on.
	 */
	void constructPaths();

	/** greebo: Adds the EnginePath and fs_game widgets to the Preference dialog
	 */
	void constructPreferences();

public:
	Manager();

	/** greebo: Reloads the setting from the registry and
	 * 			triggers a VFS refresh if the path has changed.
	 *
	 * @forced: Forces the update (don't check whether anything has changed)
	 */
	void updateEnginePath(bool forced = false);

	/** greebo: Gets the engine path (e.g. /usr/local/doom3/).
	 */
	const std::string& getEnginePath() const;

	/**
	 * greebo: Gets the mod path (e.g. ~/.doom3/gathers/).
	 * Returns the mod base path if the mod path itself is empty.
	 */
	const std::string& getModPath() const;

	/**
	 * greebo: Returns the mod base path (e.g. ~/.doom3/darkmod/),
	 * can be an empty string if fs_game_base is not set.
	 */
	const std::string& getModBasePath() const;

	/** greebo: Accessor method for the fs_game parameter
	 */
	const std::string& getFSGame() const;

	/** greebo: Accessor method for the fs_game_base parameter
	 */
	const std::string& getFSGameBase() const;

	/** greebo: Initialises the engine path from the settings in the registry.
	 * 			If nothing is found, the game file is queried.
	 */
	void initEnginePath();

	/** greebo: Returns the current Game (shared_ptr).
	 */
	virtual IGamePtr currentGame();

	/** greebo: Loads the game files and the saved settings.
	 * 			If no saved game setting is found, the user
	 * 			is asked to enter the relevant information in a Dialog.
	 */
	void initialise(const std::string& appPath);

	/** greebo: Scans the "games/" subfolder for .game description foles.
	 */
	void loadGameFiles(const std::string& appPath);

	// Returns the sorted game path list
	virtual const PathList& getVFSSearchPaths() const;

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

};

} // namespace game

#endif /*GAMEMANAGER_H_*/
