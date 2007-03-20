#include "GameDescription.h"

#include "iregistry.h"
#include "debugging/debugging.h"
#include "stream/textstream.h"
#include "error.h"

// GameDescription constructor. Create a GameDescription object from a
// xml::Document and a game file location.

GameDescription::GameDescription(const xml::Document& doc, const std::string& gameFile) : 
	_doc(doc)
{
	// Temporary workaround
	std::string type = gameFile.substr(0, gameFile.rfind(".game"));
	
	mGameFile = gameFile;

	mGameType = type;
	if (mGameType.empty()) {
	    globalErrorStream() << "Warning, 'type' attribute not found in " << mGameFile.c_str() << "\n";
	    // Fallback to default
	    mGameType = "doom3";
	}
}

const char* GameDescription::getKeyValue(const std::string& key) const {
	std::string gameXPath = std::string("//game[@type='") + mGameType + "']";
	
	xml::NodeList found = GlobalRegistry().findXPath(gameXPath);
	
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
