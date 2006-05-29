#include "xmlutil.h"

#include "XPathException.h"
#include "InvalidNodeException.h"
#include "AttributeNotFoundException.h"

#include <iostream>

namespace xml {

// Return the NodeList resulting from an XPath expression.

NodeList findPath(Document doc, std::string path) {

    // Set up the XPath context
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    
    context = xmlXPathNewContext(doc);
    if (context == NULL) {
        std::cerr << "ERROR: xml::findPath() failed to create XPath context "
                  << "when searching for " << path << std::endl;
        throw XPathException("Failed to create XPath context");
    }
    
    // Evaluate the expression
    
    const xmlChar* xpath = reinterpret_cast<const xmlChar*>(path.c_str());
    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);

    if (result == NULL) {
        std::cerr << "ERROR: xml::findPath() failed to evaluate expression "
                  << path << std::endl;
        throw XPathException("Failed to evaluate XPath expression");
    }
    
    // Construct the return vector. This may be empty if the provided XPath
    // expression does not identify any nodes.
    
    NodeList retval;
    xmlNodeSetPtr nodeset = result->nodesetval;
    for (int i = 0; i < nodeset->nodeNr; i++) {
        retval.push_back(nodeset->nodeTab[i]);
    }

    xmlXPathFreeObject(result);
    return retval;   
}

// Return the list of attributes belonging to the given node

AttributeTable getAttributes(Node node) {

    // Check input
    if (node == NULL || node->properties == NULL)
        throw InvalidNodeException("getAttributes(): invalid node parameter");
    
    // The first attribute is at node->properties, and its values are at
    // node->properties->children.

    AttributeTable retval;
    for (xmlAttrPtr attr = node->properties; attr != NULL; attr = attr->next) {
        std::string key = reinterpret_cast<const char*>(attr->name);
        std::string value = reinterpret_cast<const char*>(attr->children->content);
        retval[key] = value;
    }
    return retval;    
}

// Find and return the value of a single attribute

std::string lookupAttribute(Node node, std::string name) {

    // Check input
    if (node == NULL || node->properties == NULL)
        throw InvalidNodeException("getAttributes(): invalid node parameter");
    
    // Iterate through the chain of attributes to find the requested one.
    for (xmlAttrPtr attr = node->properties; attr != NULL; attr = attr->next) {
        if (xmlStrcmp(attr->name, reinterpret_cast<const xmlChar*>(name.c_str())) == 0) {
            return reinterpret_cast<const char*>(attr->children->content);   
        }
    }

    // Not found, throw the exception
    throw AttributeNotFoundException("lookupAttribute() failed to find attribute");
        
}

} // namespace xml
