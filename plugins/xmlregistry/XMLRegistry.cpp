
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

#include <vector>
#include <map>
#include <iostream>

#include "stringio.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>

#include "qerplugin.h"

// Needed for boost::algorithm::split		
typedef std::vector<std::string> stringParts;

// The "memory" structure for the upgrade methods
typedef std::map<const std::string, std::string> MemoryMap;
typedef std::map<RegistryKeyObserver*, std::string> KeyObserverMap;

class XMLRegistry : 
	public Registry 
{
public:
	// Radiant Module stuff
	typedef Registry Type;
	STRING_CONSTANT(Name, "*");

	// Return the static instance
	Registry* getTable() {
		return this;
	}

private:
	
	// The default import node and toplevel node
	std::string _topLevelNode;
	std::string _defaultImportNode;

	// The private pointers to the libxml2 and xmlutil objects 
	xml::Document 	_registry;
	xmlDocPtr		_origXmlDocPtr;
	xmlNodePtr		_importNode;
	
	// The "memory" as used by the upgrade methods
	MemoryMap _memory;
	
	// The map with all the keyobservers that are currently connected
	KeyObserverMap _keyObservers;

public:

	/* Constructor: 
	 * Creates an empty XML structure in the memory and adds the nodes _topLevelNode 
	 */
	XMLRegistry():
	_registry(NULL),
	_origXmlDocPtr(NULL),
	_importNode(NULL)
	{
		_topLevelNode		= "darkradiant";
		_defaultImportNode	= std::string("/") + _topLevelNode;
		
		// Create the base XML structure with the <darkradiant> top-level tag
		_origXmlDocPtr	 = xmlNewDoc(xmlCharStrdup("1.0"));
	  	_origXmlDocPtr->children = xmlNewDocNode(_origXmlDocPtr, NULL, 
	  												xmlCharStrdup(_topLevelNode.c_str()), 
	  												xmlCharStrdup(""));
	  	
	  	// Store the newly created document into the member variable _registry
		_registry = xml::Document(_origXmlDocPtr);
		_importNode = _origXmlDocPtr->children;
	}

	xml::NodeList findXPath(const std::string& path) {
		return _registry.findXPath(prepareKey(path));
	}

	/*	Checks whether a key exists in the XMLRegistry by querying the XPath
	 */
	bool keyExists(const std::string& key) {
		std::string fullKey = prepareKey(key);
		
		xml::NodeList result = _registry.findXPath(fullKey);
		return (result.size() > 0);
	}

	/*	Checks whether the key is an absolute or a relative path
	 * 	Absolute paths are returned unchanged, 
	 *  a prefix with the toplevel node (e.g. "/darkradiant") is appended to the relative ones 
	 */
	std::string prepareKey(const std::string& key) {
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
	
	/* Deletes this key and all its children, 
	 * this includes multiple instances nodes matching this key 
	 */ 
	void deleteXPath(const std::string& path) {
		// Add the toplevel node to the path if required
		std::string fullPath = prepareKey(path);
		xml::NodeList nodeList = _registry.findXPath(fullPath);
	
		if (nodeList.size() > 0) {
			for (unsigned int i = 0; i < nodeList.size(); i++) {
				// unlink and delete the node
				nodeList[i].erase();
			}
		}
	}
	
	/*	Adds a key <key> as child to <path> to the XMLRegistry (with the name attribute set to <name>)       
	 */
	xml::Node createKeyWithName(const std::string& path, const std::string& key, const std::string& name) {
		// Add the toplevel node to the path if required
		std::string fullPath = prepareKey(path);
		
		xmlNodePtr insertPoint;
		
		// Check if the insert point <path> exists, create it otherwise
		if (!keyExists(fullPath)) {
			insertPoint = createKey(fullPath);
		}
		else {
			xml::NodeList nodeList = _registry.findXPath(fullPath);
			insertPoint = nodeList[0].getNodePtr();
		}
		
		// Add the <key> to the insert point <path>
		xmlNodePtr createdNode = xmlNewChild(insertPoint, NULL, xmlCharStrdup(key.c_str()), xmlCharStrdup(""));
			
		if (createdNode != NULL) {
			addWhiteSpace(createdNode);
			
			// Create an xml::Node out of the xmlNodePtr createdNode and set the name attribute
			xml::Node node(createdNode);
			node.setAttributeValue("name", name);
			
			// Return the newly created node
			return node;
		}
		else {
			globalOutputStream() << "XMLRegistry: Critical: Could not create insert point.\n";
			return NULL;
		}
	}
	
	void addWhiteSpace(xmlNodePtr& node) {
		xmlNodePtr whitespace = xmlNewText(xmlCharStrdup("\n"));
		xmlAddSibling(node, whitespace);
	}
	
	/*	Adds a key to the XMLRegistry (without value, just the node)
	 *  The key has to be an absolute path like "/darkradiant/globals/ui/something
	 *  All required parent nodes are created automatically, if they don't exist     
	 */
	xmlNodePtr createKey(const std::string& key) {
		// Add the toplevel node to the path if required
		std::string fullKey = prepareKey(key);
		
		stringParts parts;
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
				xml::NodeList nodeList = _registry.findXPath(path);
				if (nodeList.size() > 0) {
					// node exists, set the insertPoint to this node and continue 
					insertPoint = nodeList[0].getNodePtr();
					// Set the createdNode to this point, in case this is the node to be created 
					createdNode = insertPoint;
				}
				else {
					// Node not found, insert it and store the newly created node as new insertPoint
					createdNode = xmlNewChild(insertPoint, NULL, xmlCharStrdup(parts[i].c_str()), xmlCharStrdup(""));
					//addWhiteSpace(createdNode);
					insertPoint = createdNode;
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
	std::string get(const std::string& key) {
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
	
	/* Gets a key containing a float from the registry, basically loads the string and
	 * converts it into a float via boost libraries 
	 */
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
	
	// Sets the value of a key from the registry, 
	// "/darkradiant" is automatically added if relative paths are used
	void set(const std::string& key, const std::string& value) {
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
			
			// Notify the observers, but use the unprepared key as argument!
			notifyKeyObservers(key);
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
	void importFromFile(const std::string& importFilePath, const std::string& parentKey) {
		std::string importKey = parentKey;
		
		// If an empty parentKey was passed, set it to the default import node
		if (importKey == "") {
			importKey = _defaultImportNode;
		}
		
		// Check if the importKey exists - if not: create it 
	  	std::string fullImportKey = prepareKey(importKey);
	  	
	  	//globalOutputStream() << "XMLRegistry: Looking for key " << fullImportKey.c_str() << "\n";
	  	
	  	if (!keyExists(fullImportKey)) {
	  		createKey(fullImportKey);	
	  	}
	  	
	  	// Set the "mountpoint" to its default value, the toplevel node
	  	xmlNodePtr importNode = _importNode;
	  	
	  	xml::NodeList importNodeList = _registry.findXPath(fullImportKey);
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
	void dump() const {
		xmlSaveFile("-", _origXmlDocPtr);
	}
	
	/*	Saves a specified path to the file <filename>. Use "-" if you want to write to std::out
	 */
	void exportToFile(const std::string& key, const std::string& filename) {
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
			xmlSaveFormatFile(filename.c_str(), targetDoc, 1);
					
			globalOutputStream() << "XMLRegistry: Saved " << key.c_str() << " to " << filename.c_str() << "\n";
		}
		else {
			globalOutputStream() << "XMLRegistry: Failed to save path " << fullKey.c_str() << "\n";
		}
	}
	
	/*	Copies a whole node (and all its children) from source/fromXPath to 
	 *  the base registry/toXPath
	 * 
	 *  @returns: true, if the copy action was successful
	 */
	bool copyNode(xml::Document& source, const std::string& fromXPath, const std::string& toXPath) {
		// Try to load the node from the user.xml
		xml::NodeList from = source.findXPath(fromXPath);
		
		if (from.size() > 0) {
			// Make sure the target path exists
			xmlNodePtr toNode = createKey(toXPath);
			
			// Cycle through all the results and insert them into the registry
			for (unsigned int i = 0; i<from.size(); i++) {
				if (toNode != NULL && toNode->children != NULL) {
					globalOutputStream() << "XMLRegistry: Importing nodes matching " 
					                     << fromXPath.c_str() << "\n";
					
					// Insert the data here
					xmlAddPrevSibling(toNode->children, from[i].getNodePtr());
				}
				else {
					globalOutputStream() << "XMLRegistry: Error: Could not create/find key: " 
					                     << toXPath.c_str() << "\n";
					return false;
				}
			}
			
			return true;
		}
		else {
			globalOutputStream() << "XMLRegistry: Error: Could not find fromXPath: " 
			                     << fromXPath.c_str() << "\n";
			return false;
		}
	}
	
	/*	Deletes a node (and all its children) from source/fromXPath
	 * 
	 *  @returns: true, if the copy action was successful
	 */
	bool deleteNode(xml::Document& source, const std::string& xPath) {
		// Try to load the node from the user.xml
		xml::NodeList nodeList = source.findXPath(xPath);
		
		if (nodeList.size() > 0) {
			globalOutputStream() << "XMLRegistry: Deleting nodes matching " << xPath.c_str() << "\n";

			for (unsigned int i = 0; i < nodeList.size(); i++) {
				// Delete the node
				nodeList[i].erase();
			}
		}
		
		// Return true, no matter if the node was found or not
		return true;
	}
	
	/*	Copies a value from source/fromXPath to the base registry/toXPath
	 * 
	 *  @returns: true, if the copy value action was successful
	 */
	bool copyValue(xml::Document& source, const std::string& fromXPath, const std::string& toXPath) {
		// Try to load the source node, return an empty string if nothing is found
		xml::NodeList nodeList = source.findXPath(fromXPath);
	
		// Does it even exist?
		if (nodeList.size() > 0) {
			// Load the node from the user's xml and get the value
			xml::Node node = nodeList[0];
			const std::string valueToCopy = node.getAttributeValue("value");
			
			globalOutputStream() << "XMLRegistry: Importing value: " << fromXPath.c_str() << "\n"; 
			
			// Copy the value into the base registry
			set(toXPath, valueToCopy);
			return true;
		}
		else {
			globalOutputStream() << "XMLRegistry: copy_value not possible: "
			                     << fromXPath.c_str() << " not found, action skipped!\n";
			return false;
		}
	}
	
	/*	Adds the element <nodeName> as child to all nodes that match <toXPath> 
	 *  with the name attribute set to <name>. The target document is <doc>.
	 * 
	 *  Note: This can and is intended to affect multiple nodes as well 
	 * 
	 *  @returns: true, if everything went fine, false otherwise  
	 */
	bool addNode(xml::Document& doc, const std::string& toXPath, 
	             const std::string& nodeName, const std::string& name) 
	{
		xmlNodePtr insertPoint;
		bool returnValue = true;
		
		// Obtain a list of the nodes the child nodeName will be added to
		xml::NodeList nodeList = doc.findXPath(toXPath);
		
		for (unsigned int i = 0; i<nodeList.size(); i++) {
			insertPoint = nodeList[i].getNodePtr();
			
			// Add the <nodeName> to the insert point
			xmlNodePtr createdNode = xmlNewChild(insertPoint, NULL, 
												 xmlCharStrdup(nodeName.c_str()), NULL);
			
			if (createdNode != NULL) {
				addWhiteSpace(createdNode);
				
				// Create an xml::Node out of the xmlNodePtr createdNode and set the name attribute
				xml::Node node(createdNode);
				node.setAttributeValue("name", name);
				
				returnValue &= true;
			}
			else {
				globalOutputStream() << "XMLRegistry: Critical: Could not create insert point.\n";
				returnValue = false;
			}
		}
		
		return returnValue;
	}
	
	/*	Sets the attribute <attrName> of all nodes matching <xPath> to <attrValue> 
	 *  and creates the attributes, if they are non-existant
	 * 
	 *  Note: This can and is intended to affect multiple nodes as well 
	 * 
	 *  @returns: true, if everything went fine, false otherwise  
	 */
	bool setAttribute(xml::Document& doc, const std::string& xPath, 
	             const std::string& attrName, const std::string& attrValue) 
	{
		// Obtain a list of the nodes whose attributes should be altered 
		xml::NodeList nodeList = doc.findXPath(xPath);
		
		for (unsigned int i = 0; i<nodeList.size(); i++) {
			nodeList[i].setAttributeValue(attrName, attrValue);
		}
		
		return true;
	}
	
	/*	Finds the attribute <attrName> of the node matching <xPath>  
	 *  and stores its value under the memory <memoryIndex>
	 * 
	 *  @returns: true, if everything went fine, false otherwise  
	 */
	bool saveAttribute(xml::Document& doc, const std::string& xPath, 
	             	   const std::string& attrName, const std::string& memoryIndex) 
	{
		// Obtain a list of the nodes that match the xPath criterion
		xml::NodeList nodeList = doc.findXPath(xPath);
		
		if (nodeList.size() > 0) {
			// Store the string into the specified memory location
			_memory[memoryIndex] = nodeList[0].getAttributeValue(attrName);
			return true;
		}
		else {
			globalOutputStream() << "XMLRegistry: Could not save attribute value " << attrName.c_str() 
								 << " located at " << xPath.c_str() << "\n";
			return false;
		}
	}
	
	/*	Retrieves the memory string _memory[memoryIndex] with a small existence check 
	 */
	const std::string getMemory(const std::string& memoryIndex) {
		MemoryMap::iterator it = _memory.find(memoryIndex);
		return (it != _memory.end()) ? _memory[memoryIndex] : "";
	}
	
	/*	Replaces all the memory references in a string with the respective content from the memory
	 */
	std::string parseMemoryVars(const std::string& input) {
		// output = input for the moment
		std::string output = input;
		
		// Initialise the boost::regex, not the double escapes 
		// (this should be "\{\$\d\}" and matches something like {$1} or {$23}
		boost::regex expr("\\{\\$(\\d)+\\}");
		boost::match_results<std::string::iterator> results;
		
		// The iterators for wading through the string		
		std::string::iterator start, end;
		start = output.begin();
 		end = output.end();
		
		// Loop through the matches and replace the occurrences with the respective memory vars
		while (boost::regex_search(start, end, results, expr, boost::match_default)) {
			// Obtain the replacement string via the getMemory method
			const std::string replacement = getMemory(std::string(results[1].first, results[1].second));
			
			// Replace the entire regex match results[0] with the replacement string
			output.replace(results[0].first, results[0].second, replacement);
			
			// Set the iterator to right after the match
  			start = results[0].second;
		}
 		
 		return output;
	}
	
	/*	Performs the action defined in the actionNode
	 * 
	 *  @returns: true if everything went fine, false otherwise
	 */
	bool performAction(xml::Document& userDoc, xml::Node& actionNode) {
		// Determine what action is to be performed
		const std::string actionName = actionNode.getName();
		
		// Determine which registry is the target for this operation
		const std::string target = actionNode.getAttributeValue("target");
		xml::Document& targetDoc = (target == "base") ? _registry : userDoc;
		
		// Copy a whole node (as specified by an xpath) into the base registry
		if (actionName == "copyNode") {
			const std::string fromXPath = parseMemoryVars(actionNode.getAttributeValue("fromXPath"));
			const std::string toXPath = parseMemoryVars(actionNode.getAttributeValue("toXPath"));
			
			// Copy the node from userDoc>fromXPath to the base registry>toXPath
			return copyNode(targetDoc, fromXPath, toXPath);
		}
		// Copy an attribute value from the user xml into the base registry
		else if (actionName == "copyValue") {
			const std::string fromXPath = parseMemoryVars(actionNode.getAttributeValue("fromXPath"));
			const std::string toXPath = parseMemoryVars(actionNode.getAttributeValue("toXPath"));
			
			// Copy the value from userDoc>fromXPath to the base registry>toXPath
			return copyValue(targetDoc, fromXPath, toXPath);
		}
		// Delete a whole node from the user xml (useful to clean things up before copying them)
		else if (actionName == "deleteNode") {
			const std::string xPath = parseMemoryVars(actionNode.getAttributeValue("xPath"));
						
			// Delete the value from userDoc>xPath
			return deleteNode(targetDoc, xPath);
		}
		// Add an element to a all nodes matching toXPath 
		else if (actionName == "addNode") {
			const std::string toXPath = parseMemoryVars(actionNode.getAttributeValue("toXPath"));
			const std::string nodeName = parseMemoryVars(actionNode.getAttributeValue("nodeName"));
			const std::string name = parseMemoryVars(actionNode.getAttributeValue("name"));
			
			return addNode(targetDoc, toXPath, nodeName, name);
		}
		// Set the attribute of all nodes matching xPath 
		else if (actionName == "setAttribute") {
			const std::string xPath = parseMemoryVars(actionNode.getAttributeValue("xPath"));
			const std::string attrName = parseMemoryVars(actionNode.getAttributeValue("name"));
			const std::string attrValue = parseMemoryVars(actionNode.getAttributeValue("value"));
			
			return setAttribute(targetDoc, xPath, attrName, attrValue);
		}
		// Find the attribute of the node matching xPath and save it under memoryIndex 
		else if (actionName == "saveAttribute") {
			const std::string xPath = parseMemoryVars(actionNode.getAttributeValue("xPath"));
			const std::string attrName = parseMemoryVars(actionNode.getAttributeValue("name"));
			const std::string memoryIndex = parseMemoryVars(actionNode.getAttributeValue("memoryIndex"));
			
			return saveAttribute(targetDoc, xPath, attrName, memoryIndex);
		}
		
		// No action taken, that was kind of a success :)		
		return true;
	}
	
	/*	Imports the values from userDoc into the base registry according
	 *  to the defined upgrade paths 
	 * 
	 *  @returns: true if everything went well, and false, if one or more actions
	 *  failed during the import process 
	 */
	bool performUpgrade(xml::Document& userDoc, const std::string& version) {
		// Load the upgrade information from the base registry
		std::string upgradePathXPath = std::string("user/upgradePaths/upgradePath");
		upgradePathXPath += "[@fromVersion='" + version + "']";
		xml::NodeList upgradePaths = findXPath(upgradePathXPath);
		
		// Up to now everything went fine...
		bool returnValue = true;
		
		// Any upgrade paths found?
		if (upgradePaths.size() > 0) {
			globalOutputStream() << "Upgrade path information found, upgrading...\n";
			
			// Load the upgradeItems from the registry
			xml::NodeList upgradeItems = upgradePaths[0].getChildren();
			
			// Execute the actions defined in this upgradepath
			for (unsigned int i = 0; i < upgradeItems.size(); i++) {
				// returnValue can only be true if both values are true
				returnValue &= performAction(userDoc, upgradeItems[i]);
			}
			
			return returnValue;
		}
		else {
			// Bail out
			globalOutputStream() << "XMLRegistry: Error: Could not load upgrade path information!\n";
			return false;
		}
	}
	
	// Imports the specified file and checks if an upgrade has to be made
	void importUserXML(const std::string& pathToUserXML) {
		std::string filename = pathToUserXML;
		std::string userVersion = "0.5.0";
		
		globalOutputStream() << "XMLRegistry: Importing user.xml from " << filename.c_str() << "\n";
		
		// Load the user xml to check if it can be upgraded file
		xmlDocPtr pUserDoc = xmlParseFile(filename.c_str());
	  	
	  	// Could the document be parsed or is it bogus?
	  	if (pUserDoc) {
	  		// Convert it into xml::Document and load the top-level node(s) (there should only be one)
	  		xml::Document userDoc(pUserDoc);
	  		xml::NodeList versionNode = userDoc.findXPath("/user/version");
	  		
	  		// Check if the version tag exists
	  		if (versionNode.size() > 0) {
	  			// It exists, load it and compare it to the base version
	  			userVersion = versionNode[0].getAttributeValue("value");
	  		}
	  		else {
	  			// No version tag, this definitely has to be upgraded...
	  			globalOutputStream() << "XMLRegistry: No version tag found, assuming version 0.5.0" << "\n";
	  		}
	  		
	  		std::string baseVersion = get("user/version");
	  			
	  		// Check if the upgrade is necessary
	  		if (baseVersion > userVersion) {
	  			// Yes it is necessary, userVersion < baseVersion
	  			globalOutputStream() << "XMLRegistry: Upgrading " << filename.c_str() << " from version: " 
	  		                     	 << userVersion.c_str() << " to " << baseVersion.c_str() << "\n";
	  			                     
	  			// Create a backup of the old file
	  			std::string backupFileName = filename;
	  			boost::algorithm::replace_last(backupFileName, ".xml", std::string(".") + userVersion + ".xml");
				userDoc.saveToFile(backupFileName);
	  				
	  			// Everything is loaded and backed up, call the upgrade method
	  			performUpgrade(userDoc, userVersion);
	  		}
	  		else {
	  			// Version is up to date, no upgrade necessary, import the file as it is
	  			importFromFile(filename, "");
	  		}
	  	}
	  	else {
	  		globalOutputStream() << "XMLRegistry: Critical: Could not parse " << filename.c_str() << "\n";
	  		globalOutputStream() << "XMLRegistry: Critical: File does not exist or is not valid XML!\n";
	  	}
	}
	
	// Add an observer watching the <observedKey> to the internal list of observers. 
	void addKeyObserver(RegistryKeyObserver* observer, const std::string& observedKey) {
		_keyObservers[observer] = observedKey;
	}
	
	// Removes an observer watching the <observedKey> from the internal list of observers. 
	void removeKeyObserver(RegistryKeyObserver* observer) {
		KeyObserverMap::iterator it = _keyObservers.find(observer);
   		if (it != _keyObservers.end()) {
   			_keyObservers.erase(observer);
   		}
	}
	
	// Destructor
	~XMLRegistry() {}
	
private:

	// Cycles through the key observers and notifies the ones that observe the given <changedKey>
	void notifyKeyObservers(const std::string& changedKey) {
		for (KeyObserverMap::iterator i = _keyObservers.begin(); i != _keyObservers.end(); i++) {
			RegistryKeyObserver* keyObserver = i->first;
			const std::string observedKey = i->second;
			
			// If the key matches, notify the observer
			if (observedKey == changedKey && keyObserver != NULL) {
				keyObserver->keyChanged();
			}
		}
	}
	
}; // class XMLRegistry


/* XMLRegistry dependencies class. 
 */
 
class XMLRegistryDependencies
{
};

/* Required code to register the module with the ModuleServer.
 */

#include "modulesystem/singletonmodule.h"

typedef SingletonModule<XMLRegistry, XMLRegistryDependencies> XMLRegistryModule;

// Static instance of the XMLRegistryModule
XMLRegistryModule _theXMLRegistryModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
  initialiseModule(server);
  _theXMLRegistryModule.selfRegister();
}
