
/*	This is the implementation of the XMLRegistry structure providing easy methods to store 
 * 	all kinds of information like ui state, toolbar structures and anything that fits into an XML file. 
 * 
 * 	This is the actual implementation of the abstract base class defined in iregistry.h.
 * 
 *  Note: You need to include iregistry.h in order to use the Registry like in the examples below.
 *  
 *  Example: store a global variable:
 *  	GlobalRegistry().set("user/ui/showAllLightRadii", "1");
 * 
 *  Example: retrieve a global variable 
 *  (this returns "" if the key is not found and an error is written to globalOutputStream):
 *  	std::string value = GlobalRegistry().get("user/ui/showalllightradii");
 * 
 *  Example: import an XML file into the registry (note: imported keys overwrite previous ones!) 
 * 		GlobalRegistry().importFromFile(absolute_path_to_file[, where_to_import]);
 * 
 *  Example: export a path/key to a file:
 *  	GlobalRegistry().exportToFile(node_to_export, absolute_path_to_file);
 */

#include "iregistry.h"		// The Abstract Base Class

#include <map>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "os/file.h"
#include "version.h"
#include "imodule.h"
#include "ieventmanager.h"
#include "iradiant.h"
#include "RegistryTree.h"


class XMLRegistry : 
	public Registry
{
	// The map of RegistryKeyObservers. The same observer can observe several keys, and
	// the same key can be observed by several observers, hence the multimap. 
	typedef std::multimap<const std::string, RegistryKeyObserver*> KeyObserverMap;

private:
	
	// The default import node and toplevel node
	std::string _topLevelNode;

	// The map with all the keyobservers that are currently connected
	KeyObserverMap _keyObservers;
	
	// The "install" tree, is basically treated as read-only
	RegistryTree _standardTree;
	
	// The "user" tree, this is where all the run-time changes go
	// Note: this tree is queried first for a given key
	RegistryTree _userTree;
	
	// The query counter for some statistics :)
	unsigned int _queryCounter;

public:

	/* Constructor: 
	 * Creates two empty RegistryTrees in the memory with the default toplevel node 
	 */
	XMLRegistry() :
		_topLevelNode("darkradiant"),
		_standardTree(_topLevelNode),
		_userTree(_topLevelNode),
		_queryCounter(0)
	{}

	xml::NodeList findXPath(const std::string& path) {
		// Query the user tree first
		xml::NodeList results = _userTree.findXPath(path);
		xml::NodeList stdResults = _standardTree.findXPath(path);
		
		// Append the stdResults to the results
		for (unsigned int i = 0; i < stdResults.size(); i++) {
			results.push_back(stdResults[i]);
		}
		
		_queryCounter++;
		
		return results;
	}

	/*	Checks whether a key exists in the XMLRegistry by querying the XPath
	 */
	bool keyExists(const std::string& key) {
		// Pass the query on to findXPath which queries the subtrees
		xml::NodeList result = findXPath(key);
		return (result.size() > 0);
	}

	/* Deletes this key and all its children, 
	 * this includes multiple instances nodes matching this key 
	 */ 
	void deleteXPath(const std::string& path) {
		// Add the toplevel node to the path if required
		xml::NodeList nodeList = findXPath(path);

		if (nodeList.size() > 0) {
			for (unsigned int i = 0; i < nodeList.size(); i++) {
				// unlink and delete the node
				nodeList[i].erase();
			}
		}
	}
	
	//	Adds a key <key> as child to <path> to the XMLRegistry (with the name attribute set to <name>)
	xml::Node createKeyWithName(const std::string& path, const std::string& key, const std::string& name) {
		// The key will be created in the user tree (the default tree is read-only) 
		return _userTree.createKeyWithName(path, key, name);
	}
	
	/*	Adds a key to the XMLRegistry (without value, just the node)
	 *  All required parent nodes are created automatically, if they don't exist */
	xml::Node createKey(const std::string& key) {
		return _userTree.createKey(key);
	}
	
	// Gets a key from the registry, user tree overrides default tree
	std::string get(const std::string& key) {
		
		// Pass the query to the findXPath method, which queries the user tree first
		xml::NodeList nodeList = findXPath(key);
		
		// Does it even exist?
		// It may well be the case that this returns two or more nodes that match the key criteria
		// This function always uses the first one, as the user tree should override the default tree
		if (nodeList.size() > 0) {
			// Load the first node and get the value
			xml::Node node = nodeList[0];
			return node.getAttributeValue("value");
		}
		else {
			//globalOutputStream() << "XMLRegistry: GET: Key " << fullKey.c_str() << " not found, returning empty string!\n";
			return std::string("");
		}
	}
	
	/* Gets a key containing a float from the registry, basically loads the string and
	 * converts it into a float via boost libraries */
	float getFloat(const std::string& key) {
		// Load the key
		const std::string valueStr = get(key);
		
		// Try to convert it into a float variable
		float tempFloat;
		try {
			tempFloat = boost::lexical_cast<float>(valueStr);
		}
		catch (boost::bad_lexical_cast e) {
			tempFloat = 0.0f;
		}
		
		return tempFloat;
	}
	
	/* Sets a registry key value to the given float. The floating point variable
	 * is converted via boost libraries first. */
	void setFloat(const std::string& key, const double& value) {
		
		// Try to convert the float into a string
		std::string valueStr;
		try {
			valueStr = boost::lexical_cast<std::string>(value);
		}
		catch (boost::bad_lexical_cast e) {
			valueStr = "0.0";
		}
		
		// Pass the call to set() to do the rest
		set(key, valueStr);
	}
	
	/* Gets a key containing an integer from the registry, basically loads the string and
	 * converts it into an int via boost libraries */
	int getInt(const std::string& key) {
		// Load the key
		const std::string valueStr = get(key);
		
		// Try to convert it into a float variable
		int tempInt;
		try {
			tempInt = boost::lexical_cast<int>(valueStr);
		}
		catch (boost::bad_lexical_cast e) {
			tempInt = 0;
		}
		
		return tempInt;
	}
	
	// Sets a registry key value to the given integer. The value is converted via boost libraries first.
	void setInt(const std::string& key, const int& value) {
		
		// Try to convert the int into a string
		std::string valueStr;
		try {
			valueStr = boost::lexical_cast<std::string>(value);
		}
		catch (boost::bad_lexical_cast e) {
			valueStr = "0";
		}
		
		// Pass the call to set() to do the rest
		set(key, valueStr);
	}
	
	// Sets the value of a key from the registry, 
	void set(const std::string& key, const std::string& value) {
		
		// Create or set the value in the user tree, the default tree stays untouched
		_userTree.set(key, value);
		
		// Notify the observers, but use the unprepared key as argument!
		notifyKeyObservers(key);
	}
	
	/* Appends a whole (external) XML file to the XMLRegistry. The toplevel nodes of this file
	 * are appended to _topLevelNode (e.g. <darkradiant>) if parentKey is set to the empty string "", 
	 * otherwise they are imported as a child of the specified parentKey. Choose the target tree by
	 * passing the correct enum value (e.g. treeUser for the user tree)
	 */
	void import(const std::string& importFilePath, const std::string& parentKey, Tree tree)
	{
		switch (tree) {
			case treeUser: 
				_userTree.importFromFile(importFilePath, parentKey);
				break;
			case treeStandard:
				_standardTree.importFromFile(importFilePath, parentKey);
				break;
		}
	}
	
	// Dumps the current registry to std::out, for debugging purposes
	void dump() const {
		std::cout << "User Tree:" << std::endl;
		_userTree.dump();
		std::cout << "Default Tree:" << std::endl;
		_standardTree.dump();
	}
	
	/* Saves a specified path from the user tree to the file <filename>. 
	 * Use "-" as <filename> if you want to write to std::out.
	 */
	void exportToFile(const std::string& key, const std::string& filename) {
		// Only the usertree should be exported, so pass the call to this tree
		_userTree.exportToFile(key, filename);
	}
	
	// Add an observer watching the <observedKey> to the internal list of observers. 
	void addKeyObserver(RegistryKeyObserver* observer, const std::string& observedKey) {
		_keyObservers.insert( std::make_pair(observedKey, observer) );
	}
	
	// Removes an observer watching the <observedKey> from the internal list of observers. 
	void removeKeyObserver(RegistryKeyObserver* observer) {
		
		// Traverse through the keyObserverMap and try to find the specified observer
		for (KeyObserverMap::iterator i = _keyObservers.begin(); i != _keyObservers.end(); ) {
			if (i->second == observer) {
				// Be sure to increment the iterator with a postfix ++, so that the "old" iterator is passed
				_keyObservers.erase(i++);
			}
			else {
				i++;
			}
		}
	}
	
	// Destructor
	~XMLRegistry() {
		globalOutputStream() << "XMLRegistry Shutdown: " << _queryCounter << " queries processed.\n";

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
	
private:

	// Cycles through the key observers and notifies the ones that observe the given <changedKey>
	void notifyKeyObservers(const std::string& changedKey) {
		for (KeyObserverMap::iterator it = _keyObservers.find(changedKey);
			 it != _keyObservers.upper_bound(changedKey) && it != _keyObservers.end();
			 it++)
		{
			RegistryKeyObserver* keyObserver = it->second;

			if (keyObserver != NULL) {
				keyObserver->keyChanged();
			}
		}
	}

public:
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_XMLREGISTRY);
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "XMLRegistry::initialiseModule called\n";
		
		// Load the XML files from the installation directory
		std::string base = ctx.getApplicationPath();

		try {
			// Load all of the required XML files
			import(base + "user.xml", "", Registry::treeStandard);
			//GlobalRegistry().import(base + "upgradepaths.xml", "user", Registry::treeStandard);
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
		
		const std::string userColoursFile = ctx.getSettingsPath() + "colours.xml";
		if (file_exists(userColoursFile.c_str())) {
			import(userColoursFile, "user/ui", Registry::treeUser);
		}
		
		const std::string userInputFile = ctx.getSettingsPath() + "input.xml";
		if (file_exists(userInputFile.c_str())) {
			import(userInputFile, "user/ui", Registry::treeUser);
		}
	}

}; // class XMLRegistry

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(boost::shared_ptr<XMLRegistry>(new XMLRegistry));
	
	// Initialise the streams
	const ApplicationContext& ctx = registry.getApplicationContext();
	GlobalOutputStream::instance().setOutputStream(ctx.getOutputStream());
	GlobalErrorStream::instance().setOutputStream(ctx.getOutputStream());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
