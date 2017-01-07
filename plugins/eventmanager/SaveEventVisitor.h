#pragma once

#include "EventManager.h"

#include "xmlutil/Node.h"

#include "Accelerator.h"

namespace ui
{

/* greebo: The visitor class that saves each visited event to the XMLRegistry
 *
 * The key the events are saved under is passed to the constructor of this class,
 * for example: user/ui/input is passed.
 *
 * The resulting shortcut nodes are like: user/ui/input/shortcuts/shortcut
 */
class SaveEventVisitor :
    public IEventVisitor
{
    const std::string _rootKey;

    // The node containing all the <shortcut> tags
    xml::Node _shortcutsNode;

    EventManager& _eventManager;

public:
    SaveEventVisitor(const std::string& rootKey, EventManager& eventManager) :
        _rootKey(rootKey),
        _shortcutsNode(nullptr),
        _eventManager(eventManager)
    {
        // Remove any existing shortcut definitions
        GlobalRegistry().deleteXPath(_rootKey + "//shortcuts");

        _shortcutsNode = GlobalRegistry().createKey(_rootKey + "/shortcuts");
    }

    void visit(const std::string& eventName, const IEventPtr& event)
    {
        // Only export events with non-empty name
        if (eventName.empty()) return;

        // Try to find an accelerator connected to this event
		Accelerator& accelerator = _eventManager.findAccelerator(event);

        unsigned int keyVal = accelerator.getKey();

        const std::string keyStr = keyVal != 0 ? Accelerator::getNameFromKeyCode(keyVal) : "";
        const std::string modifierStr = wxutil::Modifier::GetModifierString(accelerator.getModifiers());

        // Create a new child under the _shortcutsNode
        xml::Node createdNode = _shortcutsNode.createChild("shortcut");

        // Convert it into an xml::Node and set the attributes
        createdNode.setAttributeValue("command", eventName);
        createdNode.setAttributeValue("key", keyStr);
        createdNode.setAttributeValue("modifiers", modifierStr);

        // Add some whitespace to the node (nicer output formatting)
        createdNode.addText("\n\t");
    }

}; // class SaveEvent

}
