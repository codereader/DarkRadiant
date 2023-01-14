#pragma once

#include "xmlutil/Document.h"

namespace registry
{

const char* const TOPLEVEL_NODE_NAME = "darkradiant";

class RegistryTree
{
private:
	// The top level node name ("darkradiant")
	std::string _topLevelNode;
	std::string _defaultImportNode;

	// The actual XML data is encapsulated in this Document
	xml::Document _tree;

public:
	// Constructor, creates a default XML document
	RegistryTree();

	// Copy-Construct this tree, creates a deep copy of the other tree
	RegistryTree(const RegistryTree& other);

	// Returns a list of nodes matching the given <xpath>
	xml::NodeList findXPath(const std::string& xPath);

	//	Checks whether a key exists in the XMLRegistry by querying the XPath
	bool keyExists(const std::string& key);

	/* Deletes this key and all its children,
	 * this includes multiple instances nodes matching this key */
    std::size_t deleteXPath(const std::string& path);

	/*	Adds a key to the XMLRegistry (without value, just the node)
	 *  All required parent nodes are created automatically, if they don't exist */
	xml::Node createKey(const std::string& key);

	//	Adds a key <key> as child to <path> to the XMLRegistry (with the name attribute set to <name>)
	xml::Node createKeyWithName(const std::string& path, const std::string& key, const std::string& name);

	/**
	 * greebo: Gets a key from the registry, /darkradiant is automatically added by prepareKey()
	 * if relative paths are used.
	 *
	 * The returned value is properly converted from UTF-8 to the current locale using gtkutil::IConv.
	 */
	std::string get(const std::string& key);

	// Sets the value of a key from the registry,
	// "/darkradiant" is automatically added if relative paths are used
	// Note that the given key is NOT processed in terms of UTF-8 <-> locale conversion.
	void set(const std::string& key, const std::string& value);

	// Sets the attribute of a specified key
	void setAttribute(const std::string& path,
		const std::string& attrName, const std::string& attrValue);

	/* Appends a whole (external) XML file to the XMLRegistry. The toplevel nodes of this file
	 * are appended to _topLevelNode (e.g. <darkradiant>) if parentKey is set to the empty string "",
	 * otherwise they are imported as a child of the specified parentKey
	 */
	void importFromFile(const std::string& importFilePath, const std::string& parentKey);

	//	Saves a specified path to the file <filename>. Use "-" if you want to write to std::out
	void exportToFile(const std::string& key, const std::string& filename);

	// Dump the tree to std::out for debugging purposes
	void dump() const;

private:
	/* Checks whether the key is an absolute or a relative path
	 * Absolute paths are returned unchanged, a prefix with the
	 * toplevel node (e.g. "/darkradiant") is appended to the relative ones.
	 */
	std::string prepareKey(const std::string& key);
};

}
