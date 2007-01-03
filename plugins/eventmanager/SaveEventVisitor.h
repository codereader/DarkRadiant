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
	xmlNodePtr _shortcutsNode;
	
	IEventManager* _eventManager;
	
public:
	SaveEventVisitor(const std::string rootKey, IEventManager* eventManager) : 
		_rootKey(rootKey),
		_eventManager(eventManager)
	{
		// Remove any existing shortcut definitions
		GlobalRegistry().deleteXPath(_rootKey + "//shortcuts");
		
		_shortcutsNode = GlobalRegistry().createKey(_rootKey + "/shortcuts");
	}
	
	void visit(const std::string& eventName, IEvent* event) {
		// Sanity check 
		if (event != NULL && eventName != "") {
			// Try to find an accelerator connected to this event
			Accelerator* accelerator = dynamic_cast<Accelerator*>(_eventManager->findAccelerator(event));
			
			std::string keyStr = "";
			std::string modifierStr = "";
			
			if (accelerator != NULL) {
				keyStr = gdk_keyval_name(accelerator->getKey());
				modifierStr = _eventManager->getModifierStr(accelerator->getModifiers());
			}
			
			// Create a new child under the _shortcutsNode
			xmlNodePtr createdNodePtr = xmlNewChild(_shortcutsNode, NULL, xmlCharStrdup("shortcut"), NULL);
			
			// Convert it into an xml::Node and set the attributes
			xml::Node(createdNodePtr).setAttributeValue("command", eventName);
			xml::Node(createdNodePtr).setAttributeValue("key", keyStr);
			xml::Node(createdNodePtr).setAttributeValue("modifiers", modifierStr);
			
			// Add some whitespace to the node
			xmlNodePtr whitespace = xmlNewText(xmlCharStrdup("\n\t"));
			xmlAddSibling(createdNodePtr, whitespace);
		}
	}

}; // class SaveEvent

#endif /*SAVEEVENTVISITOR_H_*/
