#pragma once

#include "xmlutil/Node.h"
#include "imodule.h"
#include <list>
#include <string>

// String identifier for the game manager module
const char* const MODULE_GAMEMANAGER("GameManager");

const char* const RKEY_GAME_TYPE = "user/game/type";
const char* const RKEY_FS_GAME = "user/game/fs_game";
const char* const RKEY_FS_GAME_BASE = "user/game/fs_game_base";
const char* const RKEY_ENGINE_PATH = "user/paths/enginePath";
const char* const RKEY_MOD_PATH = "user/paths/modPath";
const char* const RKEY_MOD_BASE_PATH = "user/paths/modBasePath";

namespace game
{

/// Interface for an object representing a single game type.
class IGame
{
public:
    /// Destructor
    virtual ~IGame() {}

    /**
     * \brief Get a string key value from the game file.
     *
     * The "key values" are the attributes on the top-level <game> node, for example "type",
     * "name" and "index". Originally these were the only data items stored in the .game
     * file, before it was expanded into a general XML tree.
     *
     * \param key
     * Name of the key to retrieve. If this key does not exist, a warning is
     * emitted and the empty string is returned.
	 */
    virtual std::string getKeyValue(const std::string& key) const = 0;

    /**
     * @brief Test if this game has a specific optional feature.
     *
     * Games are allowed to specify certain features to expose in the UI, which might not be
     * required in other games. Each feature is defined by a single text string which appears under
     * the <features> node.
     */
    virtual bool hasFeature(const std::string& feature) const = 0;

    /// Get the name of this game
    virtual std::string getName() const = 0;

    /**
     * \brief
     * Search an XPath relative to the this game node.
     *
     * \param xpath
     * The <b>relative</b> XPath under the game node, including the initial
     * forward-slash(es).
     */
    virtual xml::NodeList getLocalXPath(const std::string& path) const = 0;
};

typedef std::shared_ptr<IGame> IGamePtr;

/**
* Represents the game configuration as specified by the user
* in the Game Settings dialog, comprising Game name,
* engine path, mod path, etc.
*/
struct GameConfiguration
{
	// The name of the current game, e.g. "Doom 3"
	std::string gameType;

	// The engine path (pointing to the game executable)
	std::string enginePath;

	// The "userengine" path (where the fs_game is stored)
	// this is ~/.doom3/<fs_game> in linux, and <enginepath>/<fs_game> in Win32
	std::string modBasePath;

	// The "mod mod" path (where the fs_game_base is stored)
	// this is ~/.doom3/<fs_game_base> in linux, and <enginepath>/<fs_game_base> in Win32
	std::string modPath;
};

/**
 * \brief
 * Interface for the game management module.
 */
class IGameManager :
	public RegisterableModule
{
public:
	// Returns the user's local engine path, on POSIX systems this might
	// point to the folder in the home directory, e.g. ~/.doom3/ if it exists.
	// If no engine directory is found in the home directory, the regular
	// engine path is returned, e.g. /usr/local/doom3 or c:\games\doom3
	virtual std::string getUserEnginePath() = 0;

	/**
	 * greebo: Gets the mod path (e.g. ~/.doom3/gathers/).
	 * Returns the mod base path if the mod path itself is empty.
	 */
	virtual const std::string& getModPath() const = 0;

	/**
	 * greebo: Returns the mod base path (e.g. ~/.doom3/darkmod/),
	 * can be an empty string if fs_game_base is not set.
	 */
	virtual const std::string& getModBasePath() const = 0;

	/** greebo: Returns the current Game (shared_ptr).
	 */
	virtual IGamePtr currentGame() = 0;

	// A sorted list of engine paths (queried by the VFS)
	typedef std::list<std::string> PathList;

	// Returns the list of ordered engine paths, which should
	// be initialised by the Virtual Filesystem (in this exact order)
	virtual const PathList& getVFSSearchPaths() const = 0;

	typedef std::vector<IGamePtr> GameList;
	virtual const GameList& getSortedGameList() = 0;

	// Returns the absolute path where maps are going to be saved to
	virtual const std::string& getMapPath() = 0;

	// Returns the absolute path where prefabs are going to be saved to
	virtual const std::string& getPrefabPath() = 0;

    // Returns the active game configuration
    virtual const GameConfiguration& getConfig() const = 0;

	// Activates the given mod configuration
	// Stores the given config, initialises VFS and constructs a few secondary paths
	virtual void applyConfig(const GameConfiguration& config) = 0;
};
typedef std::shared_ptr<IGameManager> IGameManagerPtr;

} // namespace game

// This is the accessor for the game manager
inline game::IGameManager& GlobalGameManager()
{
	static module::InstanceReference<game::IGameManager> _reference(MODULE_GAMEMANAGER);
	return _reference;
}
