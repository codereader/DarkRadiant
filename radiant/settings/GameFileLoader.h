#ifndef GAMEFILELOADER_H_
#define GAMEFILELOADER_H_

#include "os/path.h"
#include "xmlutil/Document.h"
#include "itextstream.h"

class GameDescription;

// Functor object responsible for parsing and loading an individual .game
// file.

class GameFileLoader 
{
	// Reference to external list of valid game descriptions. See the
	// GameDescription decl for more info.
	std::list<GameDescription*>& mGames;

	// Path to the game file to be added
	const char *mPath;

public:

	// Constructor
	GameFileLoader(std::list<GameDescription*>& games, const char *path) :
		mGames(games),
		mPath(path) 
	{}

	// Main functor () function
	void operator() (const char *name) const {
		// Only operate on .game files
		if (!extension_equal(path_get_extension(name), "game")) {
			return;
		}
	
		// Debug output
		std::string strPath = mPath;
		strPath += name;
		globalOutputStream() << strPath.c_str() << '\n';
	
		// Handle the XML file via libxml2
		xmlDocPtr pDoc = xmlParseFile(strPath.c_str());
	
		if (pDoc) {
			// Parse success, add to list
			std::string fileName = name;
			mGames.push_front(new GameDescription(xml::Document(pDoc), fileName));
		}
		else {
			// Error
			globalErrorStream() << "XML parser failed on '" << strPath.c_str() << "'\n";
		}
	}
};

#endif /*GAMEFILELOADER_H_*/
