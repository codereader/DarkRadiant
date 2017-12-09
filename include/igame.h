#pragma once

#include "xmlutil/Node.h"
#include "imodule.h"
#include <list>

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

/**
 * \brief
 * Interface for an object representing a single game type.
 */
class IGame
{
public:
    /**
	 * \brief
	 * Destructor
	 */
	virtual ~IGame() {}

    /**
     * \brief
     * Get a string key value from the game file.
     *
     * \param key
     * Name of the key to retrieve. If this key does not exist, a warning is
     * emitted and the empty string is returned.
	 */
    virtual std::string getKeyValue(const std::string& key) const = 0;

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
};
typedef std::shared_ptr<IGameManager> IGameManagerPtr;

} // namespace game

// This is the accessor for the game manager
inline game::IGameManager& GlobalGameManager()
{
	// Cache the reference locally
	static game::IGameManager& _gameManager(
		*std::static_pointer_cast<game::IGameManager>(
			module::GlobalModuleRegistry().getModule(MODULE_GAMEMANAGER)
		)
	);
	return _gameManager;
}
