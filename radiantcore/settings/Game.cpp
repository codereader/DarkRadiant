#include "Game.h"

#include "itextstream.h"
#include "iregistry.h"
#include "xmlutil/Document.h"
#include <iostream>

namespace game
{

const std::string Game::FILE_EXTENSION(".game");

Game::Game(const std::string& path, const std::string& filename)
{
	std::string fullPath = path + filename;

	// Load the XML file by constructing an xml::Document object
	// and search for the <game> tag
	xml::Document doc(fullPath);

	if (!doc.isValid())
	{
		rError() << "Could not parse XML file: " << fullPath << std::endl;
		return;
	}

	// Check for a toplevel game node
	xml::NodeList list = doc.findXPath("/game");

	if (list.empty())
	{
	    rError() << "Couldn't find <game> node in the game description file " << fullPath << std::endl;
		return;
	}

	const xml::Node& node = list.front();

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

	if (!_name.empty()) 
	{
		// Import the game file into the registry
		GlobalRegistry().import(fullPath, "", Registry::treeStandard);

		// Get the engine path
		_enginePath = getKeyValue(enginePath);
	}
}

Game::Game(const Game& other) :
	IGame(other),
	_enginePath(other._enginePath),
	_name(other._name)
{}

std::string Game::getName() const
{
	return _name;
}

std::string Game::getXPathRoot() const
{
	return std::string("//game[@name='") + _name + "']";
}

std::string Game::getKeyValue(const std::string& key) const
{
    if (xml::NodeList found = GlobalRegistry().findXPath(getXPathRoot()); !found.empty()) {
        return found[0].getAttributeValue(key);
    }
    else {
        rError() << "Game: Keyvalue '" << key << "' not found for game type '" << _name << "'"
                 << std::endl;
        return "";
    }
}

bool Game::hasFeature(const std::string& feature) const
{
    xml::NodeList nodes = getLocalXPath("/features");
    if (nodes.empty())
        return false;

    // Find the first available feature which matches the query feature
    xml::NodeList features = nodes[0].getNamedChildren("feature");
    for (const auto& f: features) {
        if (f.getContent() == feature)
            return true;
    }

    // Nothing found, so the optional feature isn't present
    return false;
}

xml::NodeList Game::getLocalXPath(const std::string& localPath) const
{
    std::string absolutePath = getXPathRoot() + localPath;
    return GlobalRegistry().findXPath(absolutePath);
}

} // namespace game
