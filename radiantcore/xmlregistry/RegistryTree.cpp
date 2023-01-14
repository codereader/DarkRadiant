#include "RegistryTree.h"

#include "itextstream.h"
#include "string/split.h"
#include "string/encoding.h"

namespace registry
{

RegistryTree::RegistryTree() :
	_topLevelNode(TOPLEVEL_NODE_NAME),
	_defaultImportNode(std::string("/") + _topLevelNode),
	_tree(xml::Document::create())
{
	// Create the base XML structure with the <darkradiant> top-level tag
	_tree.addTopLevelNode(_topLevelNode);
}

RegistryTree::RegistryTree(const RegistryTree& other) :
	_topLevelNode(other._topLevelNode),
	_defaultImportNode(other._defaultImportNode),
	_tree(xml::Document::clone(other._tree)) // copy-construct
{
}

std::string RegistryTree::prepareKey(const std::string& key)
{
	if (key.empty())
	{
		// no string passed, return to sender
		return key;
	}
	else if (key[0]=='/')
	{
		// this is a path relative to root, don't alter it
		return key;
	}
	else 
	{
		// add the prefix <darkradiant> and return
		return std::string("/") + _topLevelNode + std::string("/") + key;
	}
}

xml::NodeList RegistryTree::findXPath(const std::string& xPath)
{
	return _tree.findXPath(prepareKey(xPath));
}

bool RegistryTree::keyExists(const std::string& key)
{
	std::string fullKey = prepareKey(key);

	xml::NodeList result = _tree.findXPath(fullKey);
	return !result.empty();
}

std::size_t RegistryTree::deleteXPath(const std::string& path)
{
	// Add the toplevel node to the path if required
	std::string fullPath = prepareKey(path);
	xml::NodeList nodeList = _tree.findXPath(fullPath);

	for (xml::Node& node : nodeList)
	{
		// unlink and delete the node
		node.erase();
	}

    return nodeList.size();
}

xml::Node RegistryTree::createKeyWithName(const std::string& path,
										  const std::string& key,
										  const std::string& name)
{
	// Add the toplevel node to the path if required
	std::string fullPath = prepareKey(path);

	xml::Node insertPoint(nullptr, nullptr);

	// Check if the insert point <path> exists, create it otherwise
	if (!keyExists(fullPath))
	{
		insertPoint = createKey(fullPath);
	}
	else
	{
		xml::NodeList nodeList = _tree.findXPath(fullPath);
		insertPoint = nodeList[0];
	}

	// Add the <key> to the insert point <path>
	xml::Node createdNode = insertPoint.createChild(key);

	// Set the "name" attribute and return
	createdNode.setAttributeValue("name", name);

	return createdNode;
}

xml::Node RegistryTree::createKey(const std::string& key)
{
	// Add the toplevel node to the path if required
	std::string fullKey = prepareKey(key);

	std::vector<std::string> parts;
	string::split(parts, fullKey, "/");

	// Are there any slashes in the path at all? If not, exit, we've no use for this
	if (parts.empty())
	{
		rMessage() << "XMLRegistry: Cannot insert key/path without slashes." << std::endl;
		return xml::Node(nullptr, nullptr);
	}
		
	xml::Node createdNode(nullptr, nullptr);

	// The temporary path variable for walking through the hierarchy
	std::string path("");

	// Start at the root node
	xml::Node insertPoint = _tree.getTopLevelNode();

	for (const std::string& part : parts)
	{
		if (part.empty()) continue;

		// Construct the new path to be searched for
		path += "/" + part;

		// Check if the path exists
		xml::NodeList nodeList = _tree.findXPath(path);

		if (!nodeList.empty())
		{
			// node exists, set the insertPoint to this node and continue
			insertPoint = nodeList[0];
			// Set the createdNode to this point, in case this is the node to be created
			createdNode = insertPoint;
		}
		else
		{
			// Node not found, insert it and store the newly created node as new insertPoint
			createdNode = insertPoint.createChild(part);
			insertPoint = createdNode;
			createdNode.addText(" ");
		}
	}

	// return the pointer to the deepest, newly created node
	return createdNode;
}

std::string RegistryTree::get(const std::string& key)
{
	// Add the toplevel node to the path if required
	std::string fullKey = prepareKey(key);

	// Try to load the node, return an empty string if nothing is found
	xml::NodeList nodeList = _tree.findXPath(fullKey);

	// Does it even exist?
	// There is the theoretical case that this returns two nodes that match the key criteria
	// This function always uses the first one, but this may be changed if this turns out to be problematic
	if (!nodeList.empty())
	{
		// Get and convert the value
		return string::utf8_to_mb(nodeList[0].getAttributeValue("value"));
	}

	return std::string();
}

void RegistryTree::set(const std::string& key, const std::string& value)
{
	// Add the toplevel node to the path if required
	std::string fullKey = prepareKey(key);

	// If the key doesn't exist, we have to create an empty one
	if (!keyExists(fullKey)) 
	{
		createKey(fullKey);
	}

	// Try to find the node
	xml::NodeList nodeList = _tree.findXPath(fullKey);

	if (!nodeList.empty())
	{
		// Set the value
		nodeList[0].setAttributeValue("value", value);
	}
	else
	{
		// If the key is still not found, something nasty has happened
		rMessage() << "XMLRegistry: Critical: Key " << fullKey << " not found (it really should be there)!" << std::endl;
	}
}

void RegistryTree::setAttribute(const std::string& path,
		const std::string& attrName, const std::string& attrValue)
{
	// Add the toplevel node to the path if required
	std::string fullKey = prepareKey(path);

	// If the key doesn't exist, we have to create an empty one
	if (!keyExists(fullKey))
	{
		createKey(fullKey);
	}

	// Try to find the node
	xml::NodeList nodeList = _tree.findXPath(fullKey);

	if (!nodeList.empty())
	{
		// Set the value
		nodeList[0].setAttributeValue(attrName, attrValue);
	}
	else 
	{
		// If the key is still not found, something nasty has happened
		rMessage() << "XMLRegistry: Critical: Key " << fullKey << " not found (it really should be there)!" << std::endl;
	}
}

void RegistryTree::importFromFile(const std::string& importFilePath,
								  const std::string& parentKey)
{
	std::string importKey = parentKey;

	// If an empty parentKey was passed, set it to the default import node
	if (importKey.empty())
	{
		importKey = _defaultImportNode;
	}

	// Check if the importKey exists - if not: create it
  	std::string fullImportKey = prepareKey(importKey);

  	if (!keyExists(fullImportKey)) 
	{
  		createKey(fullImportKey);
  	}

  	// Lookup the mount point by using findXPath(), it must exist by now
  	xml::NodeList importNodeList = _tree.findXPath(fullImportKey);

  	if (importNodeList.empty())
	{
  		rMessage() << "XMLRegistry: Critical: ImportNode could not be found." << std::endl;
		return;
  	}

	rMessage() << "XMLRegistry: Importing XML file: " << importFilePath << std::endl;

  	// Load the file
	xml::Document importDoc(importFilePath);

  	if (!importDoc.isValid())
	{
		// Throw the XMLImportException
  		throw std::runtime_error("Unable to load file: " + importFilePath);
  	}

	// Import the document into our XML tree
	_tree.importDocument(importDoc, importNodeList[0]);
}

void RegistryTree::exportToFile(const std::string& key, const std::string& filename)
{
	if (key.empty()) return;

	// Add the toplevel node to the key if required
	std::string fullKey = prepareKey(key);

	// Try to find the specified node
	xml::NodeList result = _tree.findXPath(fullKey);

	if (result.empty()) 
	{
		rMessage() << "XMLRegistry: Failed to save path " << fullKey << std::endl;
		return;
	}

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

	// rMessage() << "XMLRegistry: Saved " << key << " to " << filename << std::endl;
}

void RegistryTree::dump() const
{
	_tree.saveToFile("-");
}

}
