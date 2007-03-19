#include "GameDescription.h"

#include "debugging/debugging.h"
#include "stream/textstream.h"
#include "error.h"

// GameDescription constructor. Create a GameDescription object from a
// xml::Document and a game file location.

GameDescription::GameDescription(const xml::Document& doc, const std::string& gameFile) : 
	_doc(doc)
{
	// Check for a toplevel game node
	xml::NodeList list = _doc.findXPath("/game");
    if (list.size() == 0) {
		Error("Didn't find 'game' node in the game description file '%s'\n", gameFile.c_str());
	}
    
	mGameFile = gameFile;

	mGameType = getKeyValue("type");
	if (mGameType.empty()) {
	    globalErrorStream() << "Warning, 'type' attribute not found in " << mGameFile.c_str() << "\n";
	    // Fallback to default
	    mGameType = "doom3";
	}
}

const char* GameDescription::getKeyValue(const std::string& key) const {
	std::string xpath = std::string("/game[@") + key + "]";
	xml::NodeList found = _doc.findXPath(xpath);
	
	if (found.size() > 0) {
		return found[0].getAttributeValue(key).c_str();
	}
	else {
		return "";
	}
}

const char* GameDescription::getRequiredKeyValue(const std::string& key) const {
	std::string returnValue = getKeyValue(key);
	if (!returnValue.empty()) {
		return returnValue.c_str();
	}
	else {
		std::string gameFile = mGameFile;
		ERROR_MESSAGE("game attribute " << makeQuoted(key.c_str()) << " not found in " << makeQuoted(gameFile.c_str()));
		return "";
	}
}
