#ifndef IGAMEMANAGER_H_
#define IGAMEMANAGER_H_

#include "imodule.h"
#include <list>

// String identifier for the game manager module
const std::string MODULE_GAMEMANAGER("GameManager");

namespace game {

class IGame
{	
public:
	/** greebo: Looks up the specified key
	 */
	virtual std::string getKeyValue(const std::string& key) = 0;
	
	/** greebo: Emits an error if the keyvalue is empty
	 */
	virtual std::string getRequiredKeyValue(const std::string& key) = 0;
};
typedef boost::shared_ptr<IGame> IGamePtr;

/**
 * Abstract base class for a registry system
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
