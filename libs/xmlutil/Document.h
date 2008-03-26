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
 *
 * The contained xmlDocPtr is automatically released on destruction 
 * of this object.
 */
class Document
{
private:

    // Contained xmlDocPtr.
    xmlDocPtr _xmlDoc;
    
public:
    // Construct a Document using the provided xmlDocPtr.
	Document(xmlDocPtr doc);

	// Construct a xml::Document from the given filename (must be the full path).
	// Use the isValid() method to check if the load was successful.
	Document(const std::string& filename);
	
	// Copy constructor
	Document(const Document& other);

	// Destructor, frees the xmlDocPtr
	~Document();
    
	// Returns TRUE if the document is ok and can be queried.
	bool isValid() const;

    // Evaluate the given XPath expression and return a NodeList of matching
    // nodes.
    NodeList findXPath(const std::string& path) const;
    
    // Saves the file to the disk via xmlSaveFile
    void saveToFile(const std::string& filename);
};

}

#endif /*DOCUMENT_H_*/
