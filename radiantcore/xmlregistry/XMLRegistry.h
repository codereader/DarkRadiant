#pragma once

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
 *  (this returns "" if the key is not found and an error is written to rMessage):
 *  	std::string value = GlobalRegistry().get("user/ui/showalllightradii");
 *
 *  Example: import an XML file into the registry (note: imported keys overwrite previous ones!)
 * 		GlobalRegistry().importFromFile(absolute_path_to_file[, where_to_import]);
 *
 *  Example: export a path/key to a file:
 *  	GlobalRegistry().exportToFile(node_to_export, absolute_path_to_file);
 */

#include "iregistry.h"
#include <map>
#include <mutex>

#include "imodule.h"
#include "RegistryTree.h"
#include "time/Timer.h"

namespace settings { class SettingsManager; }

namespace registry
{

class XMLRegistry :
	public Registry
{
private:
	// The map of registry key signals. There is one signal per key, but a
	// signal can of course be connected to multiple slots.
	typedef std::map<const std::string, sigc::signal<void> > KeySignals;
	mutable KeySignals _keySignals;

	// The "install" tree, is basically treated as read-only
	RegistryTree _standardTree;

	// The "user" tree, this is where all the run-time changes go
	// Note: this tree is queried first for a given key
	RegistryTree _userTree;

	// The query counter for some statistics :)
	unsigned int _queryCounter;

	// Change tracking counter, is reset when saveToDisk() is called
	unsigned int _changesSinceLastSave;

	// TRUE if the registry has already been saved to disk
	// At this point no more write operations should be made
	// to the registry
	bool _shutdown;

	// Auto-save helper
	std::unique_ptr<util::Timer> _autosaveTimer;

	std::mutex _writeLock;

public:
	/* Constructor:
	 * Creates two empty RegistryTrees in the memory with the default toplevel node
	 */
	XMLRegistry();

	xml::NodeList findXPath(const std::string& path) override;

	/*	Checks whether a key exists in the XMLRegistry by querying the XPath
	 */
	bool keyExists(const std::string& key) override;

	/* Deletes this key and all its children,
	 * this includes multiple instances nodes matching this key
	 */
	void deleteXPath(const std::string& path) override;

	//	Adds a key <key> as child to <path> to the XMLRegistry (with the name attribute set to <name>)
	xml::Node createKeyWithName(const std::string& path, const std::string& key, const std::string& name) override;

	/*	Adds a key to the XMLRegistry (without value, just the node)
	 *  All required parent nodes are created automatically, if they don't exist */
	xml::Node createKey(const std::string& key) override;

	// Set the value of the given attribute at the specified <path>.
	void setAttribute(const std::string& path,
		const std::string& attrName,
		const std::string& attrValue) override;

	// Loads the string value of the given attribute of the node at <path>.
	std::string getAttribute(const std::string& path, const std::string& attrName) override;

	// Gets a key from the registry, user tree overrides default tree
	std::string get(const std::string& key) override;

	// Sets the value of a key from the registry,
	void set(const std::string& key, const std::string& value) override;

	/* Appends a whole (external) XML file to the XMLRegistry. The toplevel nodes of this file
	 * are appended to _topLevelNode (e.g. <darkradiant>) if parentKey is set to the empty string "",
	 * otherwise they are imported as a child of the specified parentKey. Choose the target tree by
	 * passing the correct enum value (e.g. treeUser for the user tree)
	 */
	void import(const std::string& importFilePath, const std::string& parentKey, Tree tree) override;

	// Dumps the current registry to std::out, for debugging purposes
	void dump() const override;

	// Exports the user-tree to XML files in the user's settings path
	void saveToDisk() override;

	/* Saves a specified path from the user tree to the file <filename>.
	 * Use "-" as <filename> if you want to write to std::out.
	 */
	void exportToFile(const std::string& key, const std::string& filename) override;

	sigc::signal<void> signalForKey(const std::string& key) const override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void loadUserFileFromSettingsPath(const settings::SettingsManager& settingsManager,
		const std::string& filename, const std::string& baseXPath);

	void emitSignalForKey(const std::string& changedKey);

	// Invoked after all modules have been uninitialised
	void shutdown();

	void onAutoSaveTimerIntervalReached();
};
typedef std::shared_ptr<XMLRegistry> XMLRegistryPtr;

}
