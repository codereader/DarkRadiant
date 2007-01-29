#include "RegistryTree.h"

#include "stream/textstream.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

	namespace {
		// Needed for boost::algorithm::split		
		typedef std::vector<std::string> StringParts;
	}

// Constructor
RegistryTree::RegistryTree(const std::string& topLevelNode) :
	_tree(NULL),
	_origXmlDocPtr(NULL),
	_importNode(NULL),
	_topLevelNode(topLevelNode)
{
	_defaultImportNode = std::string("/") + _topLevelNode;
	
	// Create the base XML structure with the <darkradiant> top-level tag
	_origXmlDocPtr = xmlNewDoc(xmlCharStrdup("1.0"));
  	_origXmlDocPtr->children = xmlNewDocNode(_origXmlDocPtr, NULL, 
  											 xmlCharStrdup(_topLevelNode.c_str()), 
  											 xmlCharStrdup(""));
  	
  	// Store the newly created document into the member variable _tree
	_tree = xml::Document(_origXmlDocPtr);
	_importNode = _origXmlDocPtr->children;
}

std::string RegistryTree::prepareKey(const std::string& key) {
	std::string returnValue = key;
	
	if (returnValue.length() == 0) {
		// no string passed, return to sender
		return returnValue;
	}
	else if (returnValue[0]=='/') {
		// this is a path relative to root, don't alter it
		return returnValue;
	}
	else {
		// add the prefix <darkradiant> and return
		returnValue = std::string("/") + _topLevelNode + std::string("/") + key;
		return returnValue;
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

	if (nodeList.size() > 0) {
		for (unsigned int i = 0; i < nodeList.size(); i++) {
			// unlink and delete the node
			nodeList[i].erase();
		}
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
	
	xmlNodePtr insertPoint;
	
	// Check if the insert point <path> exists, create it otherwise
	if (!keyExists(fullPath)) {
		insertPoint = createKey(fullPath);
	}
	else {
		xml::NodeList nodeList = _tree.findXPath(fullPath);
		insertPoint = nodeList[0].getNodePtr();
	}
	
	// Add the <key> to the insert point <path>
	xmlNodePtr createdNode = xmlNewChild(insertPoint, NULL, xmlCharStrdup(key.c_str()), xmlCharStrdup(""));
		
	if (createdNode != NULL) {
		// Create an xml::Node out of the xmlNodePtr createdNode and set the name attribute
		xml::Node node(createdNode);
		node.addText("\n");
		node.setAttributeValue("name", name);
		
		// Return the newly created node
		return node;
	}
	else {
		globalOutputStream() << "XMLRegistry: Critical: Could not create insert point.\n";
		return NULL;
	}
}

/*	Adds a key to the XMLRegistry (without value, just the node)
 *  All required parent nodes are created automatically, if they don't exist     
 */
xmlNodePtr RegistryTree::createKey(const std::string& key) {
	// Add the toplevel node to the path if required
	std::string fullKey = prepareKey(key);
	
	StringParts parts;
	boost::algorithm::split(parts, fullKey, boost::algorithm::is_any_of("/"));
	
	//globalOutputStream() << "XMLRegistry: Inserting key: " << key.c_str() << "\n";
	
	xmlNodePtr createdNode = NULL;
	
	// Are there any slashes in the path at all? If not, exit, we've no use for this
	if (parts.size()>0) {
		
		// The temporary path variable for walking through the hierarchy
		std::string path("");
		
		// If the whole path does not exist, insert at the root node
		xmlNodePtr insertPoint = _importNode;
		
		for (unsigned int i = 0; i < parts.size(); i++) {
			if (parts[i] == "") continue;
						
			// Construct the new path to be searched for
			path += "/" + parts[i];
						
			// Check if the path exists
			xml::NodeList nodeList = _tree.findXPath(path);
			if (nodeList.size() > 0) {
				// node exists, set the insertPoint to this node and continue 
				insertPoint = nodeList[0].getNodePtr();
				// Set the createdNode to this point, in case this is the node to be created 
				createdNode = insertPoint;
			}
			else {
				// Node not found, insert it and store the newly created node as new insertPoint
				createdNode = xmlNewChild(insertPoint, NULL, xmlCharStrdup(parts[i].c_str()), xmlCharStrdup(""));
				insertPoint = createdNode;
				xml::Node(createdNode).addText("\n");
			}
		}
		
		// return the pointer to the deepest, newly created node
		return createdNode;
	}
	else {
		globalOutputStream() << "XMLRegistry: Cannot insert key/path without slashes.\n";
		return NULL;
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
		// Load the node and get the value
		xml::Node node = nodeList[0];
		return node.getAttributeValue("value");
	}
	else {
		//globalOutputStream() << "XMLRegistry: GET: Key " << fullKey.c_str() << " not found, returning empty string!\n";
		return std::string("");
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
		// Load the node and set the value
		xml::Node node = nodeList[0];
		node.setAttributeValue("value", value);
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
  	
  	// Set the "mountpoint" to its default value, the toplevel node
  	xmlNodePtr importNode = _importNode;
  	
  	xml::NodeList importNodeList = _tree.findXPath(fullImportKey);
  	if (importNodeList.size() > 0) {
  		importNode = importNodeList[0].getNodePtr();
  	}
  	else {
  		globalOutputStream() << "XMLRegistry: Critical: ImportNode could not be found.\n";
  	}
  	
  	globalOutputStream() << "XMLRegistry: Importing XML file: " << importFilePath.c_str() << "\n";
  	
  	// Load the file
	xmlDocPtr pImportDoc = xmlParseFile(importFilePath.c_str());
  	
  	if (pImportDoc) {
  		// Convert it into xml::Document and load the top-level node(s) (there should only be one)
  		xml::Document importDoc(pImportDoc);
  		xml::NodeList topLevelNodes = importDoc.findXPath("/*");
  		
  		if (importNode->children != NULL) {
  			
  			if (importNode->name != NULL) {
  				for (unsigned int i = 0; i < topLevelNodes.size(); i++) {
  					xmlNodePtr newNode = topLevelNodes[0].getNodePtr();
  					
  					// Add each of the imported nodes at the top to the registry
  					xmlAddPrevSibling(importNode->children, newNode);
  				}
  			}
  		}
  		else {
  			globalOutputStream() << "XMLRegistry: Critical: Could not import XML file. importNode is NULL!\n";
  		}
  	}
  	else {
  		// Throw the XMLImportException
  		throw std::runtime_error("Unable to load file: " + importFilePath);
  	}
}

/*	Saves a specified path to the file <filename>. Use "-" if you want to write to std::out
 */
void RegistryTree::exportToFile(const std::string& key, const std::string& filename) {
	// Add the toplevel node to the key if required
	std::string fullKey = prepareKey(key);
	
	// Try to find the specified node
	xml::NodeList result = _tree.findXPath(fullKey);
	
	if (result.size() > 0) {
		// Create a new XML document
		xmlDocPtr targetDoc = xmlNewDoc(xmlCharStrdup("1.0"));
		
		// Copy the node from the XMLRegistry and set it as root node of the newly created document
		xmlNodePtr exportNode = xmlCopyNode(result[0].getNodePtr(), 1);
		xmlDocSetRootElement(targetDoc, exportNode);
		
		// Save the whole document to the specified filename
		xmlSaveFormatFile(filename.c_str(), targetDoc, 1);
				
		globalOutputStream() << "XMLRegistry: Saved " << key.c_str() << " to " << filename.c_str() << "\n";
	}
	else {
		globalOutputStream() << "XMLRegistry: Failed to save path " << fullKey.c_str() << "\n";
	}
}

void RegistryTree::dump() const {
	xmlSaveFile("-", _origXmlDocPtr);	
}
