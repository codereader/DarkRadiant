#ifndef XMLUTIL_H_
#define XMLUTIL_H_

#include "XPathException.h"

#include <libxml/parser.h>
#include <libxml/xpath.h>

#include <string>
#include <vector>

namespace xml {

/* This library contains utility functions for reading and writing XML,
 * without having to manually traverse the low-level structures returned by
 * libxml2 API.
 */

// Typedefs
 
typedef xmlNodePtr Node;
typedef std::vector<Node> NodeList;

typedef xmlDocPtr Document;
    
/* Evaluate an XPath expression and return the resulting set of Nodes, or
 * NULL on error.
 */
    
NodeList findPath(Document, std::string path) throw (XPathException);
        
} // namespace xml

#endif /*XMLUTIL_H_*/
