#ifndef _UPGRADEPATH_BACKUP_H_
#define _UPGRADEPATH_BACKUP_H_

// The "memory" structure for the upgrade methods
typedef std::map<const std::string, std::string> MemoryMap;

// The "memory" as used by the upgrade methods
MemoryMap _memory;

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
				// Create an xml::Node out of the xmlNodePtr createdNode and set the name attribute
				xml::Node node(createdNode);
				node.addText("\n");
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
	 *  @returns: true  
	 */
	bool setAttribute(xml::Document& doc, const std::string& xPath, 
	             const std::string& attrName, const std::string& attrValue) 
	{
		globalOutputStream() << "XMLRegistry: Setting attribute " << attrName.c_str()
							 << " of nodes matching XPath: " << xPath.c_str() << "..."; 
		
		// Obtain a list of the nodes whose attributes should be altered 
		xml::NodeList nodeList = doc.findXPath(xPath);
		
		for (unsigned int i = 0; i<nodeList.size(); i++) {
			nodeList[i].setAttributeValue(attrName, attrValue);
		}
		
		globalOutputStream() << nodeList.size() << " nodes affected.\n";
		
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
		
		// Now replace the {$ROOT} occurence with the XMLRegistry root key
		// The regex would be \{\$ROOT\}, but the backslashes have to be escaped for C++, hence the double
		expr = "\\{\\$ROOT\\}";
		start = output.begin();
 		end = output.end();
		while (boost::regex_search(start, end, results, expr, boost::match_default)) {
 			const std::string replacement = std::string("/") + _topLevelNode;
 			
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

#endif /*_UPGRADEPATH_BACKUP_H_*/
