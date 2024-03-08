#include "Node.h"

#include <mutex>

#include "Document.h"

namespace xml
{

bool Node::isValid() const
{
    return _xmlNode;
}

std::string Node::getName() const
{
    std::lock_guard lock(_owner->getLock());

    if (_xmlNode) {
        return _xmlNode.name();
    }
    return {};
}

NodeList Node::getChildren() const
{
    // Lock the document to collect the children
    std::lock_guard lock(_owner->getLock());

    NodeList retval;
    for (auto i = _xmlNode.begin(); i != _xmlNode.end(); ++i) {
        retval.emplace_back(_owner, *i);
    }
    return retval;
}

Node Node::createChild(const std::string& name)
{
    std::lock_guard lock(_owner->getLock());

    // Create a new child under the contained node
    auto newChild = _xmlNode.append_child(name.c_str());

    // Create a new xml::Node out of this pointer and return it
    return Node(_owner, newChild);
}

NodeList Node::getNamedChildren(const std::string& name) const
{
    std::lock_guard lock(_owner->getLock());

    NodeList retval;

    // Iterate throught the list of children, adding each child node to the return list if
    // it matches the requested name
    for (auto i = _xmlNode.begin(); i != _xmlNode.end(); ++i) {
        if (i->name() == name) {
            retval.emplace_back(_owner, *i);
        }
    }

    return retval;
}

void Node::setAttributeValue(const std::string& key, const std::string& value)
{
    std::lock_guard lock(_owner->getLock());

    pugi::xml_attribute attr = _xmlNode.attribute(key.c_str());
    if (!attr)
        attr = _xmlNode.append_attribute(key.c_str());

    attr.set_value(value.c_str());
}

void Node::removeAttribute(const std::string& key)
{
    std::lock_guard lock(_owner->getLock());

    _xmlNode.remove_attribute(key.c_str());
}

std::string Node::getAttributeValue(const std::string& key) const
{
    std::lock_guard lock(_owner->getLock());

    pugi::xml_attribute attr = _xmlNode.attribute(key.c_str());
    if (attr)
        return attr.value();
    else
        return {};
}

std::string Node::getContent() const
{
    std::lock_guard lock(_owner->getLock());

    return _xmlNode.text().get();
}

void Node::setContent(const std::string& content)
{
    std::lock_guard lock(_owner->getLock());

    _xmlNode.text() = content.c_str();
}

void Node::addText(const std::string& text)
{
    std::lock_guard lock(_owner->getLock());

    // Add a PCDATA node as a sibling following this node
    auto textNode = _xmlNode.parent().insert_child_after(pugi::node_pcdata, _xmlNode);
    textNode.set_value(text.c_str());
}

void Node::erase()
{
    std::lock_guard lock(_owner->getLock());

    _xmlNode.parent().remove_child(_xmlNode);
}

pugi::xml_node Node::getNodePtr() const
{
    return _xmlNode;
}

} // namespace xml
