#include "Game.h"

#include "itextstream.h"
#include "iregistry.h"
#include "xmlutil/Document.h"
#include <iostream>

namespace game {

/** greebo: Constructor, call this with the filename relative to "games/"
 */
Game::Game(const std::string& path, const std::string& filename) {

	std::string fullPath = path + filename;

	// Load the XML file by constructing an xml::Document object
	// and search for the <game> tag
	xml::Document doc(fullPath);

	if (doc.isValid()) {
		// Check for a toplevel game node
		xml::NodeList list = doc.findXPath("/game");
	    if (list.size() == 0) {
	    	rError()
	    		<< "Couldn't find <game> node in the game description file "
	    		<< fullPath.c_str() << "\n";
		}
		else {
			xml::Node node = list[0];

			// Get the game name
			_name = node.getAttributeValue("name");

			const std::string enginePath =
#if defined(WIN32)
				"enginepath_win32"
#elif defined(__linux__) || defined (__FreeBSD__)
				"enginepath_linux"
#elif defined(__APPLE__)
				"enginepath_macos"
#else
#error "unknown platform"
#endif
			;

			if (!_name.empty()) {
				// Import the game file into the registry
				GlobalRegistry().import(fullPath, "", Registry::treeStandard);

				// Get the engine path
				_enginePath = getKeyValue(enginePath);
			}
		}
	}
	else {
		rError() << "Could not parse XML file: " << fullPath.c_str() << "\n";
	}
}

Game::Game(const Game& other) :
	IGame(other),
	_enginePath(other._enginePath),
	_name(other._name)
{}

std::string Game::getName() const {
	return _name;
}

// Get XPath root query
std::string Game::getXPathRoot() const
{
	return std::string("//game[@name='") + _name + "']";
}

// Get the specified "keyvalue" for the game. This is basically an attribute on
// the main <game> node.
std::string Game::getKeyValue(const std::string& key) const
{
	xml::NodeList found = GlobalRegistry().findXPath(getXPathRoot());

	if (!found.empty())
    {
		return found[0].getAttributeValue(key);
	}
	else
    {
		std::cout << "Game: Keyvalue '" << key
				  << "' not found for game type '" << _name << "'";
		return "";
	}
}

// Search a local XPath
xml::NodeList Game::getLocalXPath(const std::string& localPath) const
{
    std::string absolutePath = getXPathRoot() + localPath;
    return GlobalRegistry().findXPath(absolutePath);
}

} // namespace game
