#pragma once

#include "igame.h"

namespace game 
{

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
class Game: public IGame
{
	std::string _enginePath;

   // user-friendly name
	std::string _name;

public:
	// Public constant
	static const std::string FILE_EXTENSION;

	// greebo: Constructor, call this with the filename relative to "games/"
	Game(const std::string& path, const std::string& filename);

	// Copy Constructor
	Game(const Game& other);

    /* IGame implementation */
    std::string getName() const override;
    bool hasFeature(const std::string& feature) const override;
    std::string getKeyValue(const std::string& key) const override;
    xml::NodeList getLocalXPath(const std::string& path) const override;

private:
	// Return the string representing the XPath root for this game node
	std::string getXPathRoot() const;
};
typedef std::shared_ptr<Game> GamePtr;

} // namespace game
