#include "XMLRegistry.h"

#include <vector>

#include "stringio.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

namespace xml {
	
	// Constants and Types
	namespace {
		// Needed for boost::algorithm::split		
		typedef std::vector<std::string> stringParts;
	}
	
/* Constructor: 
 * Creates an empty XML structure in the memory and adds the nodes TOPLEVEL_NODE and GLOBALS_NODE 
 */
XMLRegistry::XMLRegistry():
	_registry(NULL),
	_origXmlDocPtr(NULL),
	_importNode(NULL)
{
	// Create the base XML structure with the <darkradiant> top-level tag
	_origXmlDocPtr	 = xmlNewDoc(xmlCharStrdup("1.0"));
  	_origXmlDocPtr->children = xmlNewDocNode(_origXmlDocPtr, NULL, 
  												xmlCharStrdup(TOPLEVEL_NODE.c_str()), 
  												xmlCharStrdup(""));
  	
  	// Store the newly created document into the member variable _registry
	_registry = xml::Document(_origXmlDocPtr);
	_importNode = _origXmlDocPtr->children;
}

xml::NodeList XMLRegistry::findXPath(const std::string& path) {
	return _registry.findXPath(prepareKey(path));
}

/*	Checks whether a key exists in the XMLRegistry by querying the XPath
 */
bool XMLRegistry::keyExists(const std::string& key) {
	std::string fullKey = prepareKey(key);
	
	xml::NodeList result = _registry.findXPath(fullKey);
	return (result.size() > 0);
}

/*	Checks whether the key is an absolute or a relative path
 * 	Absolute paths are returned unchanged, 
 *  a prefix with the toplevel node (e.g. "/darkradiant") is appended to the relative ones 
 */
std::string XMLRegistry::prepareKey(const std::string& key) {
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
		returnValue = std::string("/") + TOPLEVEL_NODE + std::string("/") + key;
		return returnValue;
	}
}

/*	Adds a key to the XMLRegistry (without value, just the node)
 *  The key has to be an absolute path like "/darkradiant/globals/ui/something
 *  All required parent nodes are created automatically, if they don't exist     
 */
void XMLRegistry::createKey(const std::string& key) {
	stringParts parts;
	boost::algorithm::split(parts, key, boost::algorithm::is_any_of("/"));
	
	//globalOutputStream() << "XMLRegistry: Inserting key: " << key.c_str() << "\n";
	
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
			xml::NodeList nodeList = _registry.findXPath(path);
			if (nodeList.size() > 0) {
				// node exists, set the insertPoint to this node and continue 
				insertPoint = nodeList[0].getNodePtr();
			}
			else {
				// Node not found, insert it and store the newly created node as new insertPoint 
				insertPoint = xmlNewChild(insertPoint, NULL, xmlCharStrdup(parts[i].c_str()), xmlCharStrdup(""));
			}
		}
	}
	else {
		globalOutputStream() << "XMLRegistry: Cannot insert key/path without slashes.\n";
	}
}

/* Gets a key from the registry, /darkradiant is automatically added by prepareKey()
 * if relative paths are used 
 */
std::string XMLRegistry::get(const std::string& key) {
	// Add the toplevel node to the path if required
	std::string fullKey = prepareKey(key);
	
	//globalOutputStream() << "XMLRegistry: Querying key: " << fullKey.c_str() << "\n";
	
	// Try to load the node, return an empty string if nothing is found
	xml::NodeList nodeList = _registry.findXPath(fullKey);
	
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
void XMLRegistry::set(const std::string& key, const std::string& value) {
	// Add the toplevel node to the path if required
	std::string fullKey = prepareKey(key);
	
	// If the key doesn't exist, we have to create an empty one
	if (!keyExists(fullKey)) {
		createKey(fullKey);
	}
	
	// Try to find the node	
	xml::NodeList nodeList = _registry.findXPath(fullKey);
	
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
 * are appended to TOPLEVEL_NODE (e.g. <darkradiant>) by default, 
 * otherwise they are imported as a child of parentKey
 */
void XMLRegistry::importFromFile(const std::string& importFilePath, const std::string& parentKey) {
	// Check if the parentKey exists - if not: create it 
  	std::string fullParentKey = prepareKey(parentKey);
  	
  	//globalOutputStream() << "XMLRegistry: Looking for key " << fullParentKey.c_str() << "\n";
  	
  	if (!keyExists(fullParentKey)) {
  		createKey(fullParentKey);	
  	}
  	
  	// Set the "mountpoint" to its default value, the toplevel node
  	xmlNodePtr importNode = _importNode;
  	
  	xml::NodeList importNodeList = _registry.findXPath(fullParentKey);
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
  		globalOutputStream() << "XMLRegistry: Critical: Could not parse " << importFilePath.c_str() << "\n";
  		globalOutputStream() << "XMLRegistry: Critical: File does not exist or is not valid XML!\n";
  	}
}

// Dumps the current registry to std::out, for debugging purposes
void XMLRegistry::dump() const {
	xmlSaveFile("-", _origXmlDocPtr);
}

/*	Saves a specified path to the file <filename>. Use "-" if you want to write to std::out
 */
void XMLRegistry::exportToFile(const std::string& key, const std::string& filename) {
	// Add the toplevel node to the key if required
	std::string fullKey = prepareKey(key);
	
	// Try to find the specified node
	xml::NodeList result = _registry.findXPath(fullKey);
	
	if (result.size() > 0) {
		// Create a new XML document
		xmlDocPtr targetDoc = xmlNewDoc(xmlCharStrdup("1.0"));
		
		// Copy the node from the XMLRegistry and set it as root node of the newly created document
		xmlNodePtr exportNode = xmlCopyNode(result[0].getNodePtr(), 1);
		xmlDocSetRootElement(targetDoc, exportNode);
		
		// Save the whole document to the specified filename
		xmlSaveFile(filename.c_str(), targetDoc);
		
		globalOutputStream() << "XMLRegistry: Saved " << key.c_str() << " to " << filename.c_str() << "\n";
	}
	else {
		globalOutputStream() << "XMLRegistry: Failed to save path " << fullKey.c_str() << "\n";
	}
}

// Destructor
XMLRegistry::~XMLRegistry() {} 

} // namespace xml