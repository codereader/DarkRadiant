#include "Node.h"

namespace xml
{

// Construct a Node from the given xmlNodePtr.

Node::Node(xmlNodePtr node):
    _xmlNode(node)
{
}

// Return the actual node ptr
xmlNodePtr Node::getNodePtr() const {
	return _xmlNode;
}

// Return the name of a node 
const std::string Node::getName() const {
	if (_xmlNode) {
		return std::string( reinterpret_cast<const char*>(_xmlNode->name) );
	}
	else {
		return "";
	}
}

// Return a NodeList of all children of this node
NodeList Node::getChildren() const {

    NodeList retval;
    
    // Iterate throught the list of children, adding each child node
    // to the return list if it matches the requested name
    for (xmlNodePtr child = _xmlNode->children; child != NULL; child = child->next) {
        retval.push_back(child);
    }
    
    return retval;
}

// Return a NodeList of named children of this node

NodeList Node::getNamedChildren(const std::string& name) const {

    NodeList retval;
    
    // Iterate throught the list of children, adding each child node
    // to the return list if it matches the requested name
    for (xmlNodePtr child = _xmlNode->children; child != NULL; child = child->next) {
        if (xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>(name.c_str())) == 0) {
            retval.push_back(child);
        }
    }
    
    return retval;
    
}

// Set the value of the given attribute
void Node::setAttributeValue(const std::string& key, const std::string& value) {
	xmlSetProp(_xmlNode, xmlCharStrdup(key.c_str()), xmlCharStrdup(value.c_str()));
}

// Return the value of a given attribute, or throw AttributeNotFoundException
// if the attribute does not exist.

std::string Node::getAttributeValue(const std::string& key) const {

    // Iterate through the chain of attributes to find the requested one.
    for (xmlAttrPtr attr = _xmlNode->properties; attr != NULL; attr = attr->next) {
        if (xmlStrcmp(attr->name, reinterpret_cast<const xmlChar*>(key.c_str())) == 0) {
            return reinterpret_cast<const char*>(attr->children->content);   
        }
    }

    // Not found, return an empty string
    return "";
    
}

// Return the textual content of a given node. This may be an empty string if there is no
// content available.

std::string Node::getContent() const {
	if (_xmlNode->children && _xmlNode->children->content) {
		return std::string(reinterpret_cast<const char*>(_xmlNode->children->content));
	}
	else {
		return "";
	}
}

}
