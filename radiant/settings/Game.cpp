#include "Game.h"

#include "itextstream.h"
#include "iregistry.h"
#include "xmlutil/Document.h"
#include <iostream>
#include "error.h"

namespace game {

/** greebo: Constructor, call this with the filename relative to "games/"
 */
Game::Game(const std::string& path, const std::string& filename) {
	
	std::string fullPath = path + filename;
	
	// Load the XML using libxml2 to check for the <game> tag
	xmlDocPtr pDoc = xmlParseFile(fullPath.c_str());
	
	if (pDoc) {
		xml::Document doc(pDoc);
		
		// Check for a toplevel game node
		xml::NodeList list = doc.findXPath("/game");
	    if (list.size() == 0) {
			Error("Couldn't find <game> node in the game description file '%s'\n", fullPath.c_str());
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
			
			// Get the engine path
			_enginePath = node.getAttributeValue(enginePath);
			
			if (!_type.empty()) {
				// Import the game file into the registry 
				GlobalRegistry().import(fullPath, "", Registry::treeStandard);
			}
		}
	}
	else {
		globalErrorStream() << "Could not parse XML file: " << fullPath.c_str() << "\n"; 
	}
}

Game::Game(const Game& other) :
	_enginePath(other._enginePath),
	_type(other._type)
{}

std::string Game::getType() const {
	return _type;
}

/** greebo: Looks up the specified key
 */
const char* getKeyValue(const std::string& key) {
	
}

/** greebo: Emits an error if the keyvalue is empty
 */
const char* getRequiredKeyValue(const std::string& key) {
	
}

} // namespace game
