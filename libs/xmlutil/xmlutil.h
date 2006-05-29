#ifndef XMLUTIL_H_
#define XMLUTIL_H_

#include <libxml/parser.h>
#include <libxml/xpath.h>

#include <string>
#include <vector>
#include <map>

namespace xml {

/* This library contains utility functions for reading and writing XML,
 * without having to manually traverse the low-level structures returned by
 * libxml2 API.
 */

// Typedefs and data structures
 
typedef xmlNodePtr Node;
typedef std::vector<Node> NodeList;

typedef xmlDocPtr Document;

typedef std::map<std::string, std::string> AttributeTable;


/* Evaluate an XPath expression and return the resulting set of Nodes, or
 * NULL on error.
 */
    
NodeList findPath(Document, std::string path);


/* Obtain the set of attributes belonging to the given Node
 */
 
AttributeTable getAttributes(Node node);
        

/* Lookup the value of a single specified attribute belonging to the given
 * node.
 */
 
std::string lookupAttribute(Node node, std::string name);
        
} // namespace xml

#endif /*XMLUTIL_H_*/
