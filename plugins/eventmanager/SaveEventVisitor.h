#ifndef SAVEEVENTVISITOR_H_
#define SAVEEVENTVISITOR_H_

#include "ieventmanager.h"

#include "gdk/gdkkeysyms.h"

#include "xmlutil/Node.h"

#include "Accelerator.h"

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
	
	IEventManager* _eventManager;
	
public:
	SaveEventVisitor(const std::string rootKey, IEventManager* eventManager) : 
		_rootKey(rootKey),
		_shortcutsNode(NULL),
		_eventManager(eventManager)
	{
		// Remove any existing shortcut definitions
		GlobalRegistry().deleteXPath(_rootKey + "//shortcuts");
		
		_shortcutsNode = GlobalRegistry().createKey(_rootKey + "/shortcuts");
	}
	
	void visit(const std::string& eventName, IEventPtr event) {
		// Only export events with non-empty name 
		if (eventName != "") {
			// Try to find an accelerator connected to this event
			Accelerator& accelerator = dynamic_cast<Accelerator&>(_eventManager->findAccelerator(event));
			
			unsigned int keyVal = accelerator.getKey();
			
			const std::string keyStr = (keyVal != 0) ? gdk_keyval_name(keyVal) : "";
			const std::string modifierStr = _eventManager->getModifierStr(accelerator.getModifiers());
			
			// Create a new child under the _shortcutsNode
			xml::Node createdNode = _shortcutsNode.createChild("shortcut");
			
			// Convert it into an xml::Node and set the attributes
			createdNode.setAttributeValue("command", eventName);
			createdNode.setAttributeValue("key", keyStr);
			createdNode.setAttributeValue("modifiers", modifierStr);

			// Add some whitespace to the node (nicer output formatting)			
			createdNode.addText("\n\t");
		}
	}

}; // class SaveEvent

#endif /*SAVEEVENTVISITOR_H_*/
