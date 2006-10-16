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
	
	// Add the node for storing the globals
	xmlNewChild(_origXmlDocPtr->children, NULL, xmlCharStrdup(GLOBALS_NODE.c_str()), xmlCharStrdup(""));
}

/*	Checks whether a key exists in the XMLRegistry by querying the XPath
 */
bool XMLRegistry::keyExists(const std::string& key) {	
	xml::NodeList result = _registry.findXPath(key);
	return (result.size() > 0);
}

/*	Checks whether the key is an absolute or a relative path
 * 	Absolute paths are returned unchanged, 
 *  a prefix with the toplevel node (e.g. "/darkradiant") is appended to the relative ones 
 */
std::string XMLRegistry::preparePath(const std::string& key) {
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
void XMLRegistry::insertKey(const std::string& key) {
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

/* Gets a key from the registry, /darkradiant is automatically added by preparePath()
 * if relative paths are used 
 */
std::string XMLRegistry::getXmlRegistry(const std::string& rawKey) {
	// Add the toplevel node to the path if required
	std::string key = preparePath(rawKey);
	
	// Try to load the node 
	xml::NodeList nodeList = _registry.findXPath(key);
	
	// Does it even exist?
	// There is the theoretical case that this returns two nodes that match the key criteria
	// This function always uses the first one, but this may be changed if this turns out to be problematic
	if (nodeList.size() > 0) {
		// Load the node and get the value
		xml::Node node = nodeList[0];
		return node.getAttributeValue("value");
	}
	else {
		globalOutputStream() << "XMLRegistry: GET: Key " << key.c_str() << " not found, returning empty string!\n";
		return std::string("");
	}
}

// Sets the value of a key from the registry, 
// "/darkradiant" is automatically added if relative paths are used
void XMLRegistry::setXmlRegistry(const std::string& rawKey, const std::string& value) {
	// Add the toplevel node to the path if required
	std::string key = preparePath(rawKey);
	
	// If the key doesn't exist, we have to create an empty one
	if (!keyExists(key)) {
		insertKey(key);
	}
	
	// Try to find the node	
	xml::NodeList nodeList = _registry.findXPath(key);
	
	if (nodeList.size() > 0) {
		// Load the node and set the value
		xml::Node node = nodeList[0];
		node.setAttributeValue("value", value);
	}
	else {
		// If the key is still not found, something nasty has happened
		globalOutputStream() << "XMLRegistry: Critical: Key " << key.c_str() << " not found (it really should be there)!\n";
	}
}

/* Appends a whole (external) XML file to the XMLRegistry. The toplevel nodes of this file
 * are appended to TOPLEVEL_NODE (e.g. <darkradiant>), duplicate import nodes should not be a problem
 */
void XMLRegistry::addXmlFile(const std::string& importFilePath) {
	// Load the file
	xmlDocPtr pImportDoc = xmlParseFile(importFilePath.c_str());
  	
  	globalOutputStream() << "XMLRegistry: Importing XML file: " << importFilePath.c_str() << "\n";
  	
  	if (pImportDoc) {
  		// Convert it into xml::Document and load the top-level nodes
  		xml::Document importDoc(pImportDoc);
  		xml::NodeList topLevelNodes = importDoc.findXPath("/*");
  		
  		// Cycle through all toplevel nodes and append them to the internal TOPLEVEL_NODE
  		for (unsigned int i = 0; i < topLevelNodes.size(); i++) {
  			// Add the node to the registry
  			xmlAddChild(_importNode, topLevelNodes[i].getNodePtr());
  		}
  	}
  	else {
  		globalOutputStream() << "XMLRegistry: Failed to open " << importFilePath.c_str() << "\n";	
  	}
}

// Dumps the current registry to std::out, for debugging purposes
void XMLRegistry::dumpXmlRegistry() const {
	xmlSaveFile("-", _origXmlDocPtr);
}

// Destructor
XMLRegistry::~XMLRegistry() {} 

} // namespace xml

// Create an XMLRegistry instance
xml::XMLRegistry xmlRegistry;
