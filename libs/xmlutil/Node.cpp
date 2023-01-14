#include "Node.h"

#include <mutex>
#include <libxml/parser.h>

#include "Document.h"

namespace xml
{

Node::Node(const Document* owner, xmlNodePtr node) :
    _owner(owner),
    _xmlNode(node)
{}

bool Node::isValid() const
{
    return _xmlNode != nullptr;
}

std::string Node::getName() const
{
    std::lock_guard lock(_owner->getLock());

	if (_xmlNode)
    {
		return std::string(reinterpret_cast<const char*>(_xmlNode->name));
	}

    return {};
}

NodeList Node::getChildren() const
{
    // Lock the document to collect the children
    std::lock_guard lock(_owner->getLock());

    NodeList retval;

    for (auto child = _xmlNode->children; child != nullptr; child = child->next)
    {
        retval.emplace_back(_owner, child);
    }

    return retval;
}

Node Node::createChild(const std::string& name)
{
    std::lock_guard lock(_owner->getLock());

	xmlChar* nodeName = xmlCharStrdup(name.c_str());

	// Create a new child under the contained node
	xmlNodePtr newChild = xmlNewChild(_xmlNode, nullptr, nodeName, nullptr);

	xmlFree(nodeName);

	// Create a new xml::Node out of this pointer and return it
	return Node(_owner, newChild);
}

NodeList Node::getNamedChildren(const std::string& name) const
{
    std::lock_guard lock(_owner->getLock());

    NodeList retval;

    // Iterate throught the list of children, adding each child node
    // to the return list if it matches the requested name
    for (auto child = _xmlNode->children; child != nullptr; child = child->next)
    {
        if (xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>(name.c_str())) == 0)
        {
            retval.emplace_back(_owner, child);
        }
    }

    return retval;
}

void Node::setAttributeValue(const std::string& key, const std::string& value)
{
    std::lock_guard lock(_owner->getLock());

	xmlChar* k = xmlCharStrdup(key.c_str());
	xmlChar* v = xmlCharStrdup(value.c_str());

	xmlSetProp(_xmlNode, k, v);

	xmlFree(k);
	xmlFree(v);
}

// Return the value of a given attribute, or throw AttributeNotFoundException
// if the attribute does not exist.

std::string Node::getAttributeValue(const std::string& key) const
{
    std::lock_guard lock(_owner->getLock());

    // Iterate through the chain of attributes to find the requested one.
    for (auto attr = _xmlNode->properties; attr != nullptr; attr = attr->next)
    {
        if (xmlStrcmp(attr->name, reinterpret_cast<const xmlChar*>(key.c_str())) == 0)
        {
            return reinterpret_cast<const char*>(attr->children->content);
        }
    }

    // Not found, return an empty string
    return {};
}

std::string Node::getContent() const
{
    std::lock_guard lock(_owner->getLock());

	if (_xmlNode->children && _xmlNode->children->content)
    {
		return std::string(reinterpret_cast<const char*>(_xmlNode->children->content));
	}

	return {};
}

void Node::setContent(const std::string& content)
{
    std::lock_guard lock(_owner->getLock());

    // Remove all text children first
    for (xmlNodePtr child = _xmlNode->children; child != nullptr; )
    {
        xmlNodePtr next = child->next;

        if (child->type == XML_TEXT_NODE)
        {
            xmlUnlinkNode(child);
	        xmlFreeNode(child);
        }

        child = next;
    }

    xmlNodePtr child = xmlNewText(reinterpret_cast<const xmlChar*>(content.c_str()));
    xmlAddChild(_xmlNode, child);
}

void Node::addText(const std::string& text)
{
    std::lock_guard lock(_owner->getLock());

	// Allocate a new text node
	auto whitespace = xmlNewText(reinterpret_cast<const xmlChar*>(text.c_str()));

	// Add the newly allocated text as sibling of this node
    xmlAddNextSibling(_xmlNode, whitespace);
}

void Node::erase()
{
    std::lock_guard lock(_owner->getLock());

	// unlink the node from the list first, otherwise: crashes ahead!
	xmlUnlinkNode(_xmlNode);

	// All child nodes are freed recursively
	xmlFreeNode(_xmlNode);
}

xmlNodePtr Node::getNodePtr() const
{
    return _xmlNode;
}

} // namespace xml
