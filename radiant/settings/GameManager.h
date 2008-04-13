#ifndef GAMEMANAGER_H_
#define GAMEMANAGER_H_

#include <string>
#include <map>

#include "igame.h"
#include "imodule.h"
#include "iregistry.h"
#include "Game.h"

namespace game {

/** greebo: The Manager class for keeping track
 * 			of the possible games and the current game.
 */
class Manager :
	public IGameManager,
	public RegistryKeyObserver // Observes the engine path
{
public:
	// The map containing the named Game objects 
	typedef std::map<std::string, GamePtr> GameMap;

private:	
	GameMap _games;
	
	std::string _currentGameType;
	
	// The fs_game argument (usually "darkmod")
	std::string _fsGame;
	
	// The current engine path
	std::string _enginePath;
	
	// The "userengine" path (where the fs_game is stored)
	// this is ~/.doom3/<fs_game> in linux, and <enginepath>/<fs_game> in Win32
	std::string _modPath;
	
	// The sorted list of VFS search paths (valid after module initialisation) 
	PathList _vfsSearchPaths;
	
	bool _enginePathInitialised;
	
public:
	Manager();

	/** greebo: RegistryKeyObserver implementation, gets notified 
	 * 			upon engine path changes.
	 */
	void keyChanged(const std::string& key, const std::string& val);
	
	/** greebo: Returns TRUE if the engine path exists and
	 * 			the fs_game (if it is non-empty) exists as well.
	 */
	bool settingsValid() const;
	
	/** greebo: Reloads the setting from the registry and 
	 * 			triggers a VFS refresh if the path has changed.
	 * 
	 * @forced: Forces the update (don't check whether anything has changed)
	 */
	void updateEnginePath(bool forced = false);
	
	/** greebo: Gets the engine path (e.g. /usr/local/doom3/).
	 */
	const std::string& getEnginePath() const;
	
	/** greebo: Gets the mod path (e.g. ~/.doom3/darkmod/).
	 */
	const std::string& getModPath() const;

	/** greebo: Accessor method for the fs_game parameter
	 */
	const std::string& getFSGame() const;

	/** greebo: Initialises the engine path from the settings in the registry.
	 * 			If nothing is found, the game file is queried.
	 */
	void initEnginePath();
	
	/** greebo: Returns the current Game (shared_ptr).
	 */
	virtual IGamePtr currentGame();
	
	/** greebo: Returns the type of the currently active game.
	 * 			This is a convenience method to be used when loading
	 * 			modules that require a game type like "doom3".
	 */
	const std::string& getCurrentGameType() const;
	
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

private:
	/** greebo: Get the user engine path (is OS-specific)
	 */
	std::string getUserEnginePath();

	/** greebo: Builds the paths (game engine, user game engine) with
	 * 			respect to the OS we're on.
	 */
	void constructPaths();

	/** greebo: Adds the EnginePath and fs_game widgets to the Preference dialog
	 */
	void constructPreferences();
};

} // namespace game

#endif /*GAMEMANAGER_H_*/
