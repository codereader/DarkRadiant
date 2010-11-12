#ifndef GAME_H_
#define GAME_H_

#include "igame.h"

namespace game {

	namespace {
		const std::string GAME_FILE_EXT = ".game";
	}

/**
 * \brief
 * Representation of a single .game file, managed by the GameManager.
 *
 * A Game is essentially a "view" into the XMLRegistry; it stores no data itself
 * but automatically queries the registry for values relating to its associated
 * game.
 *
 * This class also contains the code for loading a .game file and importing its
 * contents into the XMLRegistry.
 */
class Game :
	public IGame
{
	std::string _enginePath;

   // user-friendly name
	std::string _name;

private:

    // Return the string representing the XPath root for this game node
    std::string getXPathRoot() const;

public:
	/** greebo: Constructor, call this with the filename relative to "games/"
	 */
	Game(const std::string& path, const std::string& filename);

	// Copy Constructor
	Game(const Game& other);

	// Destructor
	virtual ~Game() {}

    /* IGame implementation */
	std::string getName() const;
	std::string getKeyValue(const std::string& key) const;
    xml::NodeList getLocalXPath(const std::string& path) const;

};

typedef boost::shared_ptr<Game> GamePtr;

} // namespace game

#endif /*GAME_H_*/
