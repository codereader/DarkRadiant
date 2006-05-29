#ifndef XMLUTIL_H_
#define XMLUTIL_H_

#include "Node.h"

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

typedef std::vector<Node> NodeList;

typedef xmlDocPtr Document;

typedef std::map<std::string, std::string> AttributeTable;


/* Evaluate an XPath expression and return the resulting set of Nodes, or
 * NULL on error.
 */
    
NodeList findPath(Document, const std::string& path);

} // namespace xml

#endif /*XMLUTIL_H_*/
