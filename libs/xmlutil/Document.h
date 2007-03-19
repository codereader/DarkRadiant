#ifndef DOCUMENT_H_
#define DOCUMENT_H_

#include "Node.h"

#include <libxml/parser.h>

#include <string>

namespace xml
{
    
/* Document
 * 
 * This is a wrapper class for an xmlDocPtr. It provides a function to
 * evaluate an XPath expression on the document and return the set of 
 * matching Nodes.
 */

class Document
{
private:

    // Contained xmlDocPtr.
    xmlDocPtr _xmlDoc;
    
public:

    // Construct a Document wrapper from the provided xmlDocPtr.
	Document(xmlDocPtr doc);
	
	// Copy constructor
	Document(const Document& other);
    
    // Evaluate the given XPath expression and return a NodeList of matching
    // nodes.
    NodeList findXPath(const std::string& path) const;
    
    // Saves the file to the disk via xmlSaveFile
    void saveToFile(const std::string& filename);
};

}

#endif /*DOCUMENT_H_*/
