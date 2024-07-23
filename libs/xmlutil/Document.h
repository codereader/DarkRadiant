#pragma once

#include "Node.h"

#include <pugixml.hpp>
#include <mutex>
#include <string>
#include <optional>

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
    // The document itself
    pugi::xml_document _xmlDoc;

    // Last parsing result
    std::optional<pugi::xml_parse_result> _parseResult;

    mutable std::mutex _lock;

public:
    /// Construct an empty document
    Document();

    // Construct a xml::Document from the given filename (must be the full path).
    // Use the isValid() method to check if the load was successful.
    Document(const std::string& filename);

    // Create a document from the given stream. This will read the whole
    // stream data into memory, and parse it chunk by chunk.
    Document(std::istream& stream);

    /// Copy construct
    Document(const Document& rhs);

    // Creates a new xml::Document object (allocates a new xmlDoc)
    static Document create();

    // Creates a deep copy of the given Document
    static Document clone(const Document& source);

    // Add a new toplevel node with the given name to this Document
    Node addTopLevelNode(const std::string& name);

    // Returns the top level node (or an empty Node object if none exists)
    Node getTopLevelNode() const;

    // Merges the (top-level) nodes of the <other> document into this one.
    // The insertion point in this Document is specified by <importNode>.
    void importDocument(Document& other, Node& importNode);

    // Copies the given Nodes into this document (a top level node
    // must be created beforehand)
    void copyNodes(const NodeList& nodeList);

    // Returns TRUE if the document is ok and can be queried.
    bool isValid() const;

    // Evaluate the given XPath expression and return a NodeList of matching
    // nodes.
    NodeList findXPath(const std::string& path) const;

    // Saves the file to the disk via xmlSaveFormatFile
    void saveToFile(const std::string& filename) const;

    // Saves the document to a std::string and returns it
    std::string saveToString() const;

private:
    friend class Node;

    std::mutex& getLock() const;
    void createDeclNode();
    void loadFromStream(std::istream& is);
    void saveToStream(std::ostream& os) const;
};

}
