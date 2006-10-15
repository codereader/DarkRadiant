#include "XMLRegistry.h"

#include <vector>
//#include <string>
//#include <iostream>
//#include <map>

#include "stringio.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

namespace xml {
	
	// Constants and Types
	namespace {
		typedef std::vector<std::string> stringParts;
	}
	
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

bool XMLRegistry::keyExists(const std::string& key) {	
	xml::NodeList result = _registry.findXPath(key);
	return (result.size() > 0);
}

/*bool XMLRegistry::splitKey(const std::string& keyPath, std::string& path, std::string& key) {
	stringParts parts;
	
	boost::algorithm::split(parts, key, boost::algorithm::is_any_of("/"));
	
	if (parts.size()==1) {
		
	}
	else {
		path = "";
		key = keyPath;
		return true;
	}
}*/

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

void XMLRegistry::insertKey(const std::string& key) {
	stringParts parts;
	boost::algorithm::split(parts, key, boost::algorithm::is_any_of("/"));
	
	globalOutputStream() << "XMLRegistry: Inserting key: " << key.c_str() << "\n";
	
	std::string path("");
	
	// If the whole path does not exist, insert at the root node
	xmlNodePtr insertPoint = _importNode;
	
	if (parts.size()>0) {
		for (unsigned int i = 0; i < parts.size(); i++) {
			if (parts[i] == "") continue;
						
			// Construct the new path to be searched for
			path += "/" + parts[i];
			globalOutputStream() << "XMLRegistry: Looking for node: " << path.c_str() << "\n";
			
			// Check if the path exists 
			xml::NodeList nodeList = _registry.findXPath(path);
			if (nodeList.size() > 0) {
				// set the insert node to this node 
				insertPoint = nodeList[0].getNodePtr();
				globalOutputStream() << "XMLRegistry: Found!\n";
			}
			else {
				// Node not found, insert it
				globalOutputStream() << "XMLRegistry: Not found, inserting this path: " << parts[i].c_str() << "...\n";
				insertPoint = xmlNewChild(insertPoint, NULL, xmlCharStrdup(parts[i].c_str()), xmlCharStrdup(""));
			}
		}
	}
	else {
		globalOutputStream() << "XMLRegistry: No parts.\n";
	}
}

// Gets a key from the registry, /darkradiant is automatically added if relative paths are used
std::string XMLRegistry::getXmlRegistry(const std::string& rawKey) {
	std::string key = preparePath(rawKey);
	
	xml::NodeList nodeList = _registry.findXPath(key);
	
	if (nodeList.size() > 0) {
		// Load the node and get the value
		xml::Node node = nodeList[0];
		return node.getAttributeValue("value");
	}
	else {
		globalOutputStream() << "XMLRegistry: GET: Key " << key.c_str() << " not found!\n";
		return std::string("");
	}
}

// Sets the value of a key from the registry, 
// "/darkradiant" is automatically added if relative paths are used
void XMLRegistry::setXmlRegistry(const std::string& rawKey, const std::string& value) {
	std::string key = preparePath(rawKey);
	
	if (keyExists(key)) {
		globalOutputStream() << "XMLRegistry: Key " << key.c_str() << " found.\n";
	}
	else {
		globalOutputStream() << "XMLRegistry: Key " << key.c_str() << " NOT found.\n";
		insertKey(key);		
	}
	
	xml::NodeList nodeList = _registry.findXPath(key);
	
	if (nodeList.size() > 0) {
		// Load the node and set the value
		xml::Node node = nodeList[0];
		node.setAttributeValue("value", value);
	}
	else {
		globalOutputStream() << "XMLRegistry: Critical: Key " << key.c_str() << " not found!\n";
	}
}
	
void XMLRegistry::addXmlFile(const std::string& importFilePath) {
	// Load the file
	xmlDocPtr pImportDoc = xmlParseFile(importFilePath.c_str());
  	  	
  	globalOutputStream() << "XMLRegistry: Importing XML file: " << importFilePath.c_str() << "\n";
  	
  	if (pImportDoc) {
  		// Convert it into xml::Document and load the top-level nodes
  		xml::Document importDoc(pImportDoc);
  		xml::NodeList topLevelNodes = importDoc.findXPath("/*");
  		
  		for (unsigned int i = 0; i < topLevelNodes.size(); i++) {
  			globalOutputStream() << "XMLRegistry: Found " << topLevelNodes.size();
  			globalOutputStream() << " top-level nodes.\n";
  			
  			// Add the node to the registry
  			xmlAddChild(_importNode, topLevelNodes[i].getNodePtr());
  		}
  	}
  	else {
  		globalOutputStream() << "XMLRegistry: Failed to open " << importFilePath.c_str() << "\n";	
  	}
  	
  	globalOutputStream() << "XMLRegistry: Finished importing XML file: " << importFilePath.c_str() << "\n";
}

// Dumps the current registry to std::out
void XMLRegistry::dumpXmlRegistry() const {
	xmlSaveFile("-", _origXmlDocPtr);
}

XMLRegistry::~XMLRegistry() {} 

} // namespace xml

// Create an XMLRegistry instance
xml::XMLRegistry xmlRegistry;
