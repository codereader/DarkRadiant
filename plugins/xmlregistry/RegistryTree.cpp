#include "RegistryTree.h"

#include "stream/textstream.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include "gtkutil/IConv.h"

	namespace {
		// Needed for boost::algorithm::split		
		typedef std::vector<std::string> StringParts;
	}

// Constructor
RegistryTree::RegistryTree(const std::string& topLevelNode) :
	_topLevelNode(topLevelNode),
	_defaultImportNode(std::string("/") + _topLevelNode),
	_tree(xml::Document::create())
{
	// Create the base XML structure with the <darkradiant> top-level tag
	_tree.addTopLevelNode(_topLevelNode);
}

std::string RegistryTree::prepareKey(const std::string& key) {
	if (key.empty()) {
		// no string passed, return to sender
		return key;
	}
	else if (key[0]=='/') {
		// this is a path relative to root, don't alter it
		return key;
	}
	else {
		// add the prefix <darkradiant> and return
		return std::string("/") + _topLevelNode + std::string("/") + key;
	}
}

xml::NodeList RegistryTree::findXPath(const std::string& xPath) {
	return _tree.findXPath(prepareKey(xPath));
}

/*	Checks whether a key exists in the XMLRegistry by querying the XPath
 */
bool RegistryTree::keyExists(const std::string& key) {
	std::string fullKey = prepareKey(key);
	
	xml::NodeList result = _tree.findXPath(fullKey);
	return (result.size() > 0);
}

/* Deletes this key and all its children, 
 * this includes multiple instances nodes matching this key 
 */ 
void RegistryTree::deleteXPath(const std::string& path) {
	// Add the toplevel node to the path if required
	std::string fullPath = prepareKey(path);
	xml::NodeList nodeList = _tree.findXPath(fullPath);

	for (std::size_t i = 0; i < nodeList.size(); i++) {
		// unlink and delete the node
		nodeList[i].erase();
	}
}

/*	Adds a key <key> as child to <path> to the XMLRegistry (with the name attribute set to <name>)       
 */
xml::Node RegistryTree::createKeyWithName(const std::string& path, 
										  const std::string& key, 
										  const std::string& name) 
{
	// Add the toplevel node to the path if required
	std::string fullPath = prepareKey(path);
	
	xml::Node insertPoint(NULL);
	
	// Check if the insert point <path> exists, create it otherwise
	if (!keyExists(fullPath)) {
		insertPoint = createKey(fullPath);
	}
	else {
		xml::NodeList nodeList = _tree.findXPath(fullPath);
		insertPoint = nodeList[0];
	}
	
	// Add the <key> to the insert point <path>
	xml::Node createdNode = insertPoint.createChild(key);
	
	// Set the "name" attribute and return
	createdNode.setAttributeValue("name", name);

	return createdNode;
}

/*	Adds a key to the XMLRegistry (without value, just the node)
 *  All required parent nodes are created automatically, if they don't exist     
 */
xml::Node RegistryTree::createKey(const std::string& key) {
	// Add the toplevel node to the path if required
	std::string fullKey = prepareKey(key);
	
	StringParts parts;
	boost::algorithm::split(parts, fullKey, boost::algorithm::is_any_of("/"));
	
	//globalOutputStream() << "XMLRegistry: Inserting key: " << key.c_str() << "\n";
	
	// Are there any slashes in the path at all? If not, exit, we've no use for this
	if (parts.size() > 0) {
		xml::Node createdNode(NULL);
		
		// The temporary path variable for walking through the hierarchy
		std::string path("");
		
		// Start at the root node
		xml::Node insertPoint = _tree.getTopLevelNode();
		
		for (std::size_t i = 0; i < parts.size(); i++) {
			if (parts[i] == "") continue;
			
			// Construct the new path to be searched for
			path += "/" + parts[i];
			
			// Check if the path exists
			xml::NodeList nodeList = _tree.findXPath(path);

			if (nodeList.size() > 0) {
				// node exists, set the insertPoint to this node and continue 
				insertPoint = nodeList[0];
				// Set the createdNode to this point, in case this is the node to be created 
				createdNode = insertPoint;
			}
			else {
				// Node not found, insert it and store the newly created node as new insertPoint
				createdNode = insertPoint.createChild(parts[i]);
				insertPoint = createdNode;
				createdNode.addText(" ");
			}
		}
		
		// return the pointer to the deepest, newly created node
		return createdNode;
	}
	else {
		globalOutputStream() << "XMLRegistry: Cannot insert key/path without slashes.\n";
		return xml::Node(NULL);
	}
}

/* Gets a key from the registry, /darkradiant is automatically added by prepareKey()
 * if relative paths are used 
 */
std::string RegistryTree::get(const std::string& key) {
	// Add the toplevel node to the path if required
	std::string fullKey = prepareKey(key);
	
	//globalOutputStream() << "XMLRegistry: Querying key: " << fullKey.c_str() << "\n";
	
	// Try to load the node, return an empty string if nothing is found
	xml::NodeList nodeList = _tree.findXPath(fullKey);
	
	// Does it even exist?
	// There is the theoretical case that this returns two nodes that match the key criteria
	// This function always uses the first one, but this may be changed if this turns out to be problematic
	if (nodeList.size() > 0) {
		// Get and convert the value
		return gtkutil::IConv::localeFromUTF8(nodeList[0].getAttributeValue("value"));
	}
	else {
		//globalOutputStream() << "XMLRegistry: GET: Key " << fullKey.c_str() << " not found, returning empty string!\n";
		return "";
	}
}

// Sets the value of a key from the registry, 
// "/darkradiant" is automatically added if relative paths are used
void RegistryTree::set(const std::string& key, const std::string& value) {
	// Add the toplevel node to the path if required
	std::string fullKey = prepareKey(key);
	
	// If the key doesn't exist, we have to create an empty one
	if (!keyExists(fullKey)) {
		createKey(fullKey);
	}
	
	// Try to find the node	
	xml::NodeList nodeList = _tree.findXPath(fullKey);
	
	if (nodeList.size() > 0) {
		// Set the value
		nodeList[0].setAttributeValue("value", value);
	}
	else {
		// If the key is still not found, something nasty has happened
		globalOutputStream() << "XMLRegistry: Critical: Key " << fullKey.c_str() << " not found (it really should be there)!\n";
	}
}

/* Appends a whole (external) XML file to the XMLRegistry. The toplevel nodes of this file
 * are appended to _topLevelNode (e.g. <darkradiant>) if parentKey is set to the empty string "", 
 * otherwise they are imported as a child of the specified parentKey
 */
void RegistryTree::importFromFile(const std::string& importFilePath, 
								  const std::string& parentKey) 
{
	std::string importKey = parentKey;
	
	// If an empty parentKey was passed, set it to the default import node
	if (importKey == "") {
		importKey = _defaultImportNode;
	}
	
	// Check if the importKey exists - if not: create it 
  	std::string fullImportKey = prepareKey(importKey);
  	
  	if (!keyExists(fullImportKey)) {
  		createKey(fullImportKey);	
  	}
  	
  	// Lookup the mount point by using findXPath(), it must exist by now
  	xml::NodeList importNodeList = _tree.findXPath(fullImportKey);
	
  	if (importNodeList.empty()) {
  		globalOutputStream() << "XMLRegistry: Critical: ImportNode could not be found.\n";
		return;
  	}
 
  	globalOutputStream() << "XMLRegistry: Importing XML file: " << importFilePath.c_str() << "\n";
  	
  	// Load the file
	xml::Document importDoc(importFilePath);
	  	
  	if (importDoc.isValid()) {
		// Import the document into our XML tree
		_tree.importDocument(importDoc, importNodeList[0]);
  	}
  	else {
  		// Throw the XMLImportException
  		throw std::runtime_error("Unable to load file: " + importFilePath);
  	}
}

/*	Saves a specified path to the file <filename>. Use "-" if you want to write to std::cout
 */
void RegistryTree::exportToFile(const std::string& key, const std::string& filename) {
	if (key.empty()) return;
	
	// Add the toplevel node to the key if required
	std::string fullKey = prepareKey(key);
	
	// Try to find the specified node
	xml::NodeList result = _tree.findXPath(fullKey);
	
	if (result.size() > 0) {
		// Create a new xml::Document
		xml::Document targetDoc = xml::Document::create();
		
		std::string keyName = fullKey.substr(fullKey.rfind("/") + 1);
		
		// Add an empty toplevel node with the given key (leaf) name
		targetDoc.addTopLevelNode(keyName);

		// Select all the child nodes of the export key
		xml::NodeList children = _tree.findXPath(fullKey + "/*");
		
		// Copy the child nodes into this document
		targetDoc.copyNodes(children);

		// Save the whole document to the specified filename
		targetDoc.saveToFile(filename);
		
		globalOutputStream() << "XMLRegistry: Saved " << key.c_str() << " to " << filename.c_str() << "\n";
	}
	else {
		globalOutputStream() << "XMLRegistry: Failed to save path " << fullKey.c_str() << "\n";
	}
}

void RegistryTree::dump() const {
	_tree.saveToFile("-");
}
