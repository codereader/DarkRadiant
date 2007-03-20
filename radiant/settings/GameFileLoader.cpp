#include "GameFileLoader.h"

#include <list>
#include "xmlutil/Document.h"
#include "itextstream.h"
#include "iregistry.h"

#include "GameDescription.h"

// Constructor
GameFileLoader::GameFileLoader(std::list<GameDescription*>& games, const char *path) :
	mGames(games),
	mPath(path) 
{}

// Main functor () function
void GameFileLoader::operator() (const char *name) const {
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
		
		// Import the game file into the registry
		GlobalRegistry().import(strPath, "", Registry::treeUser);
	}
	else {
		// Error
		globalErrorStream() << "XML parser failed on '" << strPath.c_str() << "'\n";
	}
}
