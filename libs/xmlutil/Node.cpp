#include "Node.h"
#include "AttributeNotFoundException.h"

namespace xml
{

// Construct a Node from the given xmlNodePtr.

Node::Node(xmlNodePtr node):
    _xmlNode(node)
{
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

// Return the value of a given attribute, or throw AttributeNotFoundException
// if the attribute does not exist.

std::string Node::getAttributeValue(const std::string& key) const {

    // Iterate through the chain of attributes to find the requested one.
    for (xmlAttrPtr attr = _xmlNode->properties; attr != NULL; attr = attr->next) {
        if (xmlStrcmp(attr->name, reinterpret_cast<const xmlChar*>(key.c_str())) == 0) {
            return reinterpret_cast<const char*>(attr->children->content);   
        }
    }

    // Not found, throw the exception
    throw AttributeNotFoundException("Node::getAttributeValue() : attribute \"" + key + "\" not found");
    
}

}
