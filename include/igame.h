#ifndef IGAMEMANAGER_H_
#define IGAMEMANAGER_H_

#include "imodule.h"

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
	virtual std::string getModPath() const = 0;
	
	/** greebo: Returns the current Game (shared_ptr).
	 */
	virtual IGamePtr currentGame() = 0;
};
typedef boost::shared_ptr<IGameManager> IGameManagerPtr;

} // namespace game

// This is the accessor for the game manager
inline game::IGameManager& GlobalGameManager() {
	game::IGameManagerPtr _gameManager(
		boost::static_pointer_cast<game::IGameManager>(
			module::GlobalModuleRegistry().getModule(MODULE_GAMEMANAGER)
		)
	);
	return *_gameManager;
}

#endif /*IGAMEMANAGER_H_*/
