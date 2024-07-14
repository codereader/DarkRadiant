#pragma once

#include <pugixml.hpp>
#include <string>
#include <vector>

namespace xml
{

// Typedefs

class Node;
typedef std::vector<Node> NodeList;

class Document;

/* Node
 *
 * A representation of an XML node. This class wraps a pugixml node
 * and provides certain methods to access properties of the node.
 */
class Node
{
    const Document* _owner = nullptr;

    // The contained pugi::xml_node. This points to part of a wider document
    // structure which is not owned by this Node object.
    pugi::xml_node _xmlNode;

public:
    /// Construct a Node with no contained data
    Node(const Document* owner = nullptr): _owner(owner)
    {}

    /// Construct a Node from a provided pugi::xml_node
    Node(const Document* owner, pugi::xml_node node): _owner(owner), _xmlNode(node)
    {}

    Node(const Node& other) = default;
    Node(Node&& other) = default;

    Node& operator=(const Node& other) = default;
    Node& operator=(Node&& other) = default;

    // Returns true if this node is not empty (null)
    bool isValid() const;

    // Get the name of the given node
    std::string getName() const;

    // Get a list of nodes which are children of this node
    NodeList getChildren() const;

    // Creates a new child under this XML Node
    Node createChild(const std::string& name);

    // Get a list of nodes which are children of this node and match the
    // given name.
    NodeList getNamedChildren(const std::string& name) const;

    // Return the value of the given attribute, or an empty string
    // if the attribute is not present on this Node.
    std::string getAttributeValue(const std::string& key) const;

    // Set the value of the given attribute
    void setAttributeValue(const std::string& key, const std::string& value);

    /// Remove an attribute altogether
    void removeAttribute(const std::string& key);

    /// Return the text content of this node.
    std::string getContent() const;

    // Sets the contents of this XML node to the given string
    void setContent(const std::string& content);

    void addText(const std::string& text);

    // Unlink and delete the node and all its children
    void erase();

private:
    friend class Document;

    // Private accessor, used by the friend Document class
    pugi::xml_node getNodePtr() const;
};

} // namespace xml
