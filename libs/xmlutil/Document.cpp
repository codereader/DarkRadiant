#include "Document.h"
#include "XPathException.h"

#include "itextstream.h"
#include <vector>
#include <fstream>

namespace xml
{

Document::Document()
{
    createDeclNode();
}

Document::Document(const std::string& filename)
{
    std::ifstream fileStream(filename);
    loadFromStream(fileStream);
}

Document::Document(std::istream& stream)
{
    loadFromStream(stream);
}

Document::Document(const Document& rhs)
: _parseResult(rhs._parseResult)
{
    std::lock_guard<std::mutex> lock(rhs._lock);

    _xmlDoc.reset(rhs._xmlDoc);
}

void Document::loadFromStream(std::istream& is)
{
    _parseResult = _xmlDoc.load(is);
    createDeclNode();
}

void Document::createDeclNode()
{
    pugi::xml_node decl = _xmlDoc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "utf-8";
}

Document Document::create()
{
    return Document();
}

Document Document::clone(const Document& source)
{
    return Document(source);
}

Node Document::addTopLevelNode(const std::string& name)
{
    std::lock_guard<std::mutex> lock(_lock);

    _xmlDoc.remove_children();
    createDeclNode(); // remove_children also removes the decl
    auto node = _xmlDoc.append_child(name.c_str());

    return Node(this, node);
}

Node Document::getTopLevelNode() const
{
    std::lock_guard lock(_lock);

    if (!isValid())
        // Invalid Document, return a NULL node
        return Node(this);

    return Node(this, _xmlDoc.document_element());
}

void Document::importDocument(Document& other, Node& importNode)
{
    std::lock_guard<std::mutex> lock(_lock);

    if (!importNode.isValid()) {
        // invalid importnode
        return;
    }

    auto targetNode = importNode.getNodePtr();
    for (pugi::xml_node child: other._xmlDoc.children()) {
        targetNode.append_copy(child);
    }
}

void Document::copyNodes(const NodeList& nodeList)
{
    std::lock_guard<std::mutex> lock(_lock);

    if (!isValid()) {
        return; // is not Valid, place an assertion here?
    }

    // Copy the child nodes one by one
    for (auto node: nodeList) {
        _xmlDoc.document_element().append_copy(node.getNodePtr());
    }
}

bool Document::isValid() const
{
    return !_parseResult || _parseResult->status == pugi::status_ok;
}

// Evaluate an XPath expression and return matching Nodes.
NodeList Document::findXPath(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(_lock);

    // Evaluate the XPath
    pugi::xpath_node_set nodes = _xmlDoc.select_nodes(path.c_str());

    // Construct the return vector. This may be empty if the provided XPath
    // expression does not identify any nodes.
    NodeList retval;
    for (pugi::xpath_node xpNode: nodes) {
        retval.emplace_back(this, xpNode.node());
    }
    return retval;
}

void Document::saveToStream(std::ostream& os) const
{
    _xmlDoc.save(os, "", pugi::format_raw, pugi::encoding_utf8);
}

void Document::saveToFile(const std::string& filename) const
{
    std::lock_guard<std::mutex> lock(_lock);

    std::ofstream fileStream(filename);
    saveToStream(fileStream);
}

std::string Document::saveToString() const
{
    std::lock_guard<std::mutex> lock(_lock);

    std::stringstream stream;
    saveToStream(stream);
    return stream.str();
}

std::mutex& Document::getLock() const
{
    return _lock;
}

}
