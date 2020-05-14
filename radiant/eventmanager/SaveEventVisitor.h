#pragma once

#include "ieventmanager.h"
#include "iregistry.h"

#include "xmlutil/Node.h"
#include "wxutil/Modifier.h"
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
class SaveEventVisitor
{
    // The node containing all the <shortcut> tags
    xml::Node _shortcutsNode;

public:
    SaveEventVisitor(const std::string& rootKey) :
        _shortcutsNode(nullptr)
    {
        // Remove any existing shortcut definitions
        GlobalRegistry().deleteXPath(rootKey + "//shortcuts");

        _shortcutsNode = GlobalRegistry().createKey(rootKey + "/shortcuts");
    }

    void visit(const std::string& eventName, const IAccelerator& accelerator)
    {
        // Only export events with non-empty name
        if (eventName.empty()) return;

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
