#pragma once

#include "ui/ieventmanager.h"
#include "iregistry.h"

#include "xmlutil/Node.h"
#include "wxutil/Modifier.h"
#include "Accelerator.h"

namespace ui
{

/* greebo: Visitor class saving all bound shortcuts to the XMLRegistry
 *
 * The registry key root is passed to the constructor of this class,
 * for example: "user/ui/input", which results in the shortcuts
 * saved as: user/ui/input/shortcuts//shortcut
 */
class ShortcutSaver
{
private:
    // The node containing all the <shortcut> tags
    xml::Node _shortcutsNode;

public:
    ShortcutSaver(const std::string& rootKey) :
        _shortcutsNode(nullptr, nullptr)
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
