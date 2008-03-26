#include "Game.h"

#include "itextstream.h"
#include "iregistry.h"
#include "xmlutil/Document.h"
#include <iostream>
#include "stream/textstream.h"

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
	    	globalErrorStream() 
	    		<< "Couldn't find <game> node in the game description file "
	    		<< fullPath.c_str() << "\n";
		}
		else {
			xml::Node node = list[0];
			
			// Get the game name
			_type = node.getAttributeValue("type");
			
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
			
			if (!_type.empty()) {
				// Import the game file into the registry 
				GlobalRegistry().import(fullPath, "", Registry::treeStandard);
				
				// Get the engine path
				_enginePath = getKeyValue(enginePath);
			}
		}
	}
	else {
		globalErrorStream() << "Could not parse XML file: " << fullPath.c_str() << "\n"; 
	}
}

Game::Game(const Game& other) :
	IGame(other),
	_enginePath(other._enginePath),
	_type(other._type)
{}

std::string Game::getType() const {
	return _type;
}

/** greebo: Looks up the specified key
 */
std::string Game::getKeyValue(const std::string& key) {
	std::string gameXPath = std::string("//game[@type='") + _type + "']";
	
	xml::NodeList found = GlobalRegistry().findXPath(gameXPath);
	
	if (found.size() > 0) {
		return found[0].getAttributeValue(key);
	}
	else {
		return "";
	}
}

/** greebo: Emits an error if the keyvalue is empty
 */
std::string Game::getRequiredKeyValue(const std::string& key) {
	std::string returnValue = getKeyValue(key);
	if (!returnValue.empty()) {
		return returnValue;
	}
	else {
		std::cout << "Game: Required Keyvalue '" << key 
				  << "' not found for game type '" << _type << "'";
		return "";
	}
}

} // namespace game
