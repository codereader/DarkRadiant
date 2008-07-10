#include "XMLRegistry.h"		// The Abstract Base Class

#include <iostream>
#include <stdexcept>

#include "os/file.h"
#include "os/path.h"

#include "version.h"
#include "string/string.h"
#include "gtkutil/IConv.h"

XMLRegistry::XMLRegistry() :
	_topLevelNode("darkradiant"),
	_standardTree(_topLevelNode),
	_userTree(_topLevelNode),
	_queryCounter(0)
{}

XMLRegistry::~XMLRegistry() {
	globalOutputStream() << "XMLRegistry Shutdown: " << _queryCounter << " queries processed.\n";

	// Don't save these paths into the xml files.
	deleteXPath(RKEY_APP_PATH);
	deleteXPath(RKEY_HOME_PATH);
	deleteXPath(RKEY_SETTINGS_PATH);
	deleteXPath(RKEY_BITMAPS_PATH);

    // Application-relative on other OS
	std::string settingsPath = 
		module::GlobalModuleRegistry().getApplicationContext().getSettingsPath();

	// Save the user tree to the settings path, this contains all
	// settings that have been modified during runtime
	if (get(RKEY_SKIP_REGISTRY_SAVE).empty()) {
		// Replace the version tag and set it to the current DarkRadiant version
		deleteXPath("user//version");
		set("user/version", RADIANT_VERSION);
		
		// Export the colour schemes and remove them from the registry
		exportToFile("user/ui/colourschemes", settingsPath + "colours.xml");
		deleteXPath("user/ui/colourschemes");
		
		// Export the input definitions into the user's settings folder and remove them as well
		exportToFile("user/ui/input", settingsPath + "input.xml");
		deleteXPath("user/ui/input");
		
		// Delete all nodes marked as "transient", they are NOT exported into the user's xml file
		deleteXPath("user/*[@transient='1']");
		
		// Remove any remaining upgradePaths (from older registry files)
		deleteXPath("user/upgradePaths");
		// Remove legacy <interface> node
		deleteXPath("user/ui/interface");
		
		// Save the remaining /darkradiant/user tree to user.xml so that the current settings are preserved
		exportToFile("user", settingsPath + "user.xml");
	}
}

xml::NodeList XMLRegistry::findXPath(const std::string& path) {
	// Query the user tree first
	xml::NodeList results = _userTree.findXPath(path);
	xml::NodeList stdResults = _standardTree.findXPath(path);
	
	// Append the stdResults to the results
	for (std::size_t i = 0; i < stdResults.size(); i++) {
		results.push_back(stdResults[i]);
	}
	
	_queryCounter++;
	
	return results;
}


void XMLRegistry::dump() const {
	std::cout << "User Tree:" << std::endl;
	_userTree.dump();
	std::cout << "Default Tree:" << std::endl;
	_standardTree.dump();
}

void XMLRegistry::exportToFile(const std::string& key, const std::string& filename) {
	// Only the usertree should be exported, so pass the call to this tree
	_userTree.exportToFile(key, filename);
}

void XMLRegistry::addKeyObserver(RegistryKeyObserver* observer, const std::string& observedKey) {
	_keyObservers.insert( std::make_pair(observedKey, observer) );
}

// Removes an observer watching the <observedKey> from the internal list of observers. 
void XMLRegistry::removeKeyObserver(RegistryKeyObserver* observer) {
	// Traverse through the keyObserverMap and try to find the specified observer
    for (KeyObserverMap::iterator i = _keyObservers.begin();
         i != _keyObservers.end();
         /* in-loop increment */) 
    {
		if (i->second == observer) {
			_keyObservers.erase(i++);
		}
		else {
			i++;
		}
	}
}

bool XMLRegistry::keyExists(const std::string& key) {
	// Pass the query on to findXPath which queries the subtrees
	xml::NodeList result = findXPath(key);
	return (result.size() > 0);
}

void XMLRegistry::deleteXPath(const std::string& path) {
	// Add the toplevel node to the path if required
	xml::NodeList nodeList = findXPath(path);

	for (std::size_t i = 0; i < nodeList.size(); i++) {
		// unlink and delete the node
		nodeList[i].erase();
	}
}

xml::Node XMLRegistry::createKeyWithName(const std::string& path, const std::string& key, const std::string& name) {
	// The key will be created in the user tree (the default tree is read-only) 
	return _userTree.createKeyWithName(path, key, name);
}

xml::Node XMLRegistry::createKey(const std::string& key) {
	return _userTree.createKey(key);
}

std::string XMLRegistry::get(const std::string& key) {
	// Pass the query to the findXPath method, which queries the user tree first
	xml::NodeList nodeList = findXPath(key);
	
	// Does it even exist?
	// It may well be the case that this returns two or more nodes that match the key criteria
	// This function always uses the first one, as the user tree should override the default tree
	if (nodeList.size() > 0) {
		// Load the first node and get the value
		xml::Node node = nodeList[0];
		// Convert the UTF-8 string back to locale and return
		return gtkutil::IConv::localeFromUTF8(node.getAttributeValue("value"));
	}
	else {
		//globalOutputStream() << "XMLRegistry: GET: Key " << fullKey.c_str() << " not found, returning empty string!\n";
		return "";
	}
}

float XMLRegistry::getFloat(const std::string& key) {
	// Load the key and convert to float
	return strToFloat(get(key));
}
	
void XMLRegistry::setFloat(const std::string& key, const double& value) {
	// Pass the call to set() to do the rest
	set(key, doubleToStr(value));
}

int XMLRegistry::getInt(const std::string& key) {
	// Load the key and convert to int
	return strToInt(get(key));
}
	
void XMLRegistry::setInt(const std::string& key, const int& value) {
	// Pass the call to set() to do the rest
	set(key, intToStr(value));
}

void XMLRegistry::set(const std::string& key, const std::string& value) {
	// Create or set the value in the user tree, the default tree stays untouched
	// Convert the string to UTF-8 before storing it into the RegistryTree
	_userTree.set(key, gtkutil::IConv::localeToUTF8(value));
	
	// Notify the observers
	notifyKeyObservers(key, value);
}

void XMLRegistry::import(const std::string& importFilePath, const std::string& parentKey, Tree tree) {
	switch (tree) {
		case treeUser: 
			_userTree.importFromFile(importFilePath, parentKey);
			break;
		case treeStandard:
			_standardTree.importFromFile(importFilePath, parentKey);
			break;
	}
}

void XMLRegistry::notifyKeyObservers(const std::string& changedKey, const std::string& newVal) 
{
	for (KeyObserverMap::iterator it = _keyObservers.find(changedKey);
		 it != _keyObservers.upper_bound(changedKey) && it != _keyObservers.end();
		 it++)
	{
		RegistryKeyObserver* keyObserver = it->second;

		if (keyObserver != NULL) {
			keyObserver->keyChanged(changedKey, newVal);
		}
	}
}

// RegisterableModule implementation
const std::string& XMLRegistry::getName() const {
	static std::string _name(MODULE_XMLREGISTRY);
	return _name;
}

const StringSet& XMLRegistry::getDependencies() const {
	static StringSet _dependencies; // no dependencies
	return _dependencies;
}

void XMLRegistry::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "XMLRegistry::initialiseModule called\n";
	
#if defined(POSIX) && defined(PKGDATADIR)
    // Use "/usr/share/darkradiant" (default) on POSIX
    std::string base = os::standardPathWithSlash(PKGDATADIR);
    std::cout << "XMLRegistry: looking for XML files under " 
              << base << std::endl;
#else
	// Load the XML files from the installation directory
	std::string base = ctx.getApplicationPath();
#endif

	try {
		// Load all of the required XML files
		import(base + "user.xml", "", Registry::treeStandard);
		import(base + "colours.xml", "user/ui", Registry::treeStandard);
		import(base + "input.xml", "user/ui", Registry::treeStandard);
		import(base + "menu.xml", "user/ui", Registry::treeStandard);
		
		// Load the debug.xml file only if the relevant key is set in user.xml
		if (get("user/debug") == "1") {
			import(base + "debug.xml", "", Registry::treeStandard);
		}
	}
	catch (std::runtime_error e) {
		std::cerr << "XML registry population failed:\n\n" << e.what() << "\n";
		/*gtkutil::fatalErrorDialog("XML registry population failed:\n\n"
								  + std::string(e.what()),
								  MainFrame_getWindow());*/
	}
	
	// Load user preferences, these overwrite any values that have defined before
	// The called method also checks for any upgrades that have to be performed
	const std::string userSettingsFile = ctx.getSettingsPath() + "user.xml";
	if (file_exists(userSettingsFile.c_str())) {
		import(userSettingsFile, "", Registry::treeUser);
	}
    else {
        globalOutputStream() << 
            "XMLRegistry: no user.xml in " << userSettingsFile << "\n";
    }
	
	const std::string userColoursFile = ctx.getSettingsPath() + "colours.xml";
	if (file_exists(userColoursFile.c_str())) {
		import(userColoursFile, "user/ui", Registry::treeUser);
	}
	
	const std::string userInputFile = ctx.getSettingsPath() + "input.xml";
	if (file_exists(userInputFile.c_str())) {
		import(userInputFile, "user/ui", Registry::treeUser);
	}

	// Now the registry is up and running, tell the context to emit
	// the the relevant paths to the XMLRegistry
	ctx.savePathsToRegistry();
}
