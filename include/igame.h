#ifndef IGAMEMANAGER_H_
#define IGAMEMANAGER_H_

#include "imodule.h"
#include <list>

// String identifier for the game manager module
const std::string MODULE_GAMEMANAGER("GameManager");

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
     * Get a string key value from the game file.
     *
     * \param key
     * Name of the key to retrieve. If this key does not exist, a warning is
     * emitted and the empty string is returned.
	 */
    virtual std::string getKeyValue(const std::string& key) const = 0;
};

typedef boost::shared_ptr<IGame> IGamePtr;

/**
 * \brief
 * Interface for the game management module.
 */
class IGameManager :
	public RegisterableModule
{
public:
	/** greebo: Gets the mod path (e.g. ~/.doom3/darkmod/).
	 */
	virtual const std::string& getModPath() const = 0;
	
	/** greebo: Returns the current Game (shared_ptr).
	 */
	virtual IGamePtr currentGame() = 0;
	
	// A sorted list of engine paths (queried by the VFS)
	typedef std::list<std::string> PathList;
	
	// Returns the list of ordered engine paths, which should 
	// be initialised by the Virtual Filesystem (in this exact order) 
	virtual const PathList& getVFSSearchPaths() const = 0;  
};
typedef boost::shared_ptr<IGameManager> IGameManagerPtr;

} // namespace game

// This is the accessor for the game manager
inline game::IGameManager& GlobalGameManager() {
	// Cache the reference locally
	static game::IGameManager& _gameManager(
		*boost::static_pointer_cast<game::IGameManager>(
			module::GlobalModuleRegistry().getModule(MODULE_GAMEMANAGER)
		)
	);
	return _gameManager;
}

#endif /*IGAMEMANAGER_H_*/
