#ifndef GAME_H_
#define GAME_H_

#include "igame.h"

namespace game {
	
	namespace {
		const std::string GAME_FILE_EXT = ".game";
	}

/** greebo: A game representation containing the enigne path.
 * 			Offers methods to get the key/values from the game
 * 			description (which is wrapped into the XMLRegistry).
 */
class Game :
	public IGame
{
	std::string _enginePath;
	
	std::string _type;
	
public:
	/** greebo: Constructor, call this with the filename relative to "games/"
	 */
	Game(const std::string& path, const std::string& filename);
	
	// Copy Constructor
	Game(const Game& other);
	
	/** greebo: Retrieves the name of the game (e.g. "doom3");
	 */
	std::string getType() const;
	
	/** greebo: Looks up the specified key
	 */
	std::string getKeyValue(const std::string& key);
	
	/** greebo: Emits an error if the keyvalue is empty
	 */
	std::string getRequiredKeyValue(const std::string& key);
};

typedef boost::shared_ptr<Game> GamePtr;

} // namespace game

#endif /*GAME_H_*/
