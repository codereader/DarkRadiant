#include "ieventmanager.h"

#include "iregistry.h"
#include <iostream>
#include <typeinfo>

#include "gdk/gdkevents.h"
#include "gdk/gdkkeysyms.h"
#include "gtk/gtkwindow.h"
#include "gtk/gtkaccelgroup.h"

#include "xmlutil/Node.h"

#include "Command.h"
#include "Toggle.h"
#include "WidgetToggle.h"
#include "RegistryToggle.h"
#include "KeyEvent.h"
#include "Accelerator.h"
#include "SaveEventVisitor.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

class EventManager : 
	public IEventManager 
{
	// The handler ID of the connected keyboard handler
	typedef std::map<gulong, GtkObject*> HandlerMap;
	
	// Needed for boost::algorithm::split
	typedef std::vector<std::string> StringParts;

	typedef std::map<const std::string, unsigned int> ModifierBitIndexMap;

	// Each command has a name, this is the map where the name->command association is stored
	typedef std::map<const std::string, IEventPtr> EventMap;
	
	// The list of all allocated Accelerators
	typedef std::list<Accelerator> AcceleratorList;

	HandlerMap _handlers;
	
	// The list containing all registered accelerator objects
	AcceleratorList _accelerators;
	
	// The map of all registered events
	EventMap _events;
	
	// The list of all modifier bit indices
	ModifierBitIndexMap _modifierBitIndices;
	
	// The GTK accelerator group for the main window
	GtkAccelGroup* _accelGroup;
	
	IEventPtr _emptyEvent;
	Accelerator _emptyAccelerator;

public:
	// Radiant Module stuff
	typedef IEventManager Type;
	STRING_CONSTANT(Name, "*");

	// Return the static instance
	IEventManager* getTable() {
		return this;
	}
	
	// Constructor
	EventManager() :
		_emptyEvent(new Event()),
		_emptyAccelerator(0, 0, _emptyEvent)
	{
		// Deactivate the empty event, so it's safe to return it as NullEvent
		_emptyEvent->setEnabled(false);
		
		globalOutputStream() << "EventManager started.\n";
		_accelGroup = gtk_accel_group_new();
		
		loadModifierDefinitions();
	}
	
	// Destructor, free all allocated objects
	~EventManager() {
		globalOutputStream() << "EventManager successfully shut down.\n";
	}
	
	IAccelerator& addAccelerator(const std::string& key, const std::string& modifierStr) {
		guint keyVal = getGDKCode(key);
		unsigned int modifierFlags = getModifierFlags(modifierStr); 
		
		Accelerator accel(keyVal, modifierFlags, _emptyEvent);
		
		// Add a new Accelerator to the list
		_accelerators.push_back(accel);
		
		// return the reference to the last accelerator in the list
		AcceleratorList::reverse_iterator i = _accelerators.rbegin();
		
		return (*i);
	}
	
	IEventPtr findEvent(const std::string& name) {
		// Try to lookup the command
		EventMap::iterator i = _events.find(name);
		
		if (i != _events.end()) {
			// Return the pointer to the command
			return i->second;
		}
		else {
			// Nothing found, return the NullEvent
			return _emptyEvent;
		}
	}
	
	// Checks if the eventName is already registered and writes to globalOutputStream, if so 
	bool alreadyRegistered(const std::string& eventName) {
		// Try to find the command and see if it's already registered
		IEventPtr foundEvent = findEvent(eventName);
		
		if (foundEvent->empty()) {
			return false;
		}
		else {
			globalOutputStream() << "EventManager: Warning: Event " << eventName.c_str() << " already registered!\n";
			return true;
		}
	}
	
	// Add the given command to the internal list
	IEventPtr addCommand(const std::string& name, const Callback& callback) {
		
		if (!alreadyRegistered(name)) {
			// Add the command to the list (implicitly cast the pointer on Event&)
			IEvent* event = new Command(callback);  
			_events[name] = IEventPtr(event);
			
			// Return the pointer to the newly created event
			return _events[name];
		}
		
		return _emptyEvent;
	}
	
	IEventPtr addKeyEvent(const std::string& name, const Callback& keyUpCallback, const Callback& keyDownCallback) {
		
		if (!alreadyRegistered(name)) {
			// Add the new keyevent to the list (implicitly cast onto Event&)  
			_events[name] = IEventPtr(new KeyEvent(keyUpCallback, keyDownCallback));
			
			// Return the pointer to the newly created event
			return _events[name];
		}
		
		return _emptyEvent;
	}
	
	IEventPtr addWidgetToggle(const std::string& name) {
		
		if (!alreadyRegistered(name)) {
			// Add the command to the list (implicitly cast the pointer on Event&)  
			_events[name] = IEventPtr(new WidgetToggle());
			
			// Return the pointer to the newly created event
			return _events[name];
		}
		
		return _emptyEvent;
	}
	
	IEventPtr addRegistryToggle(const std::string& name, const std::string& registryKey) {
		
		if (!alreadyRegistered(name)) {
			// Add the command to the list (implicitly cast the pointer on Event&)  
			_events[name] = IEventPtr(new RegistryToggle(registryKey));
			
			// Return the pointer to the newly created event
			return _events[name];
		}
		
		return _emptyEvent;
	}
	
	IEventPtr addToggle(const std::string& name, const Callback& onToggled) {
		
		if (!alreadyRegistered(name)) {
			// Add the command to the list (implicitly cast the pointer on Event&)  
			_events[name] = IEventPtr(new Toggle(onToggled));
			
			// Return the pointer to the newly created event
			return _events[name];
		}
		
		return _emptyEvent;
	}
	
	void setToggled(const std::string& name, const bool toggled) {
		// Check could be placed here by boost::shared_ptr's dynamic_pointer_cast 
		if (!findEvent(name)->setToggled(toggled)) {
			std::cout << "EventManager: Warning: Event " << name.c_str() << " is not a Toggle.\n";
		}
	}
	
	// Connects the given accelerator to the given command (identified by the string)
	void connectAccelerator(IAccelerator& accelerator, const std::string& command) {
		IEventPtr event = findEvent(command);
			
		if (!event->empty()) {
			// Command found, connect it to the accelerator by passing its pointer			
			accelerator.connectEvent(event);
		}
		else {
			// Command NOT found
			globalOutputStream() << "EventManager: Unable to lookup command: " << command.c_str() << "\n";
		}
	}
	
	void disableEvent(const std::string& eventName) {
		findEvent(eventName)->setEnabled(false);
	}
	
	void enableEvent(const std::string& eventName) {
		findEvent(eventName)->setEnabled(true);
	}

	// Catches the keypress/keyrelease events from the given GtkObject
	void connect(GtkObject* object)	{
		// Create and store the handler into the map
		gulong handlerId = g_signal_connect(G_OBJECT(object), "key_press_event", G_CALLBACK(onKeyPress), this);
		_handlers[handlerId] = object;
		
		handlerId = g_signal_connect(G_OBJECT(object), "key_release_event", G_CALLBACK(onKeyRelease), this);
		_handlers[handlerId] = object;
	}
	
	void disconnect(GtkObject* object) {
		for (HandlerMap::iterator i = _handlers.begin(); i != _handlers.end(); i++) {
			if (i->second == object) {
				g_signal_handler_disconnect(G_OBJECT(i->second), i->first);
				_handlers.erase(i);
			}
		}
	}
	
	void connectAccelGroup(GtkWindow* window) {
		gtk_window_add_accel_group(window, _accelGroup); 
	}
	
	// Loads the default shortcuts from the registry
	void loadAccelerators() {
		xml::NodeList shortcutSets = GlobalRegistry().findXPath("user/ui/input//shortcuts");
		
		// If we have two sets of shortcuts, delete the default ones
		if (shortcutSets.size() > 1) {
			GlobalRegistry().deleteXPath("user/ui/input//shortcuts[@name='default']");
		}
		
		// Find all accelerators
		xml::NodeList shortcutList = GlobalRegistry().findXPath("user/ui/input/shortcuts//shortcut");
		
		if (shortcutList.size() > 0) {
			globalOutputStream() << "EventManager: Shortcuts found in Registry: " << shortcutList.size() << "\n";
			for (unsigned int i = 0; i < shortcutList.size(); i++) {
				const std::string key = shortcutList[i].getAttributeValue("key");
				
				// Try to lookup the command
				IEventPtr event = findEvent(shortcutList[i].getAttributeValue("command"));
				
				// Check for a non-empty key string
				if (key != "") {
					 // Check for valid command definitions were found
					if (!event->empty()) {
						// Get the modifier string (e.g. "SHIFT+ALT")
						const std::string modifierStr = shortcutList[i].getAttributeValue("modifiers");
						
						if (!duplicateAccelerator(key, modifierStr, event)) {
							// Create the accelerator object
							IAccelerator& accelerator = addAccelerator(key, modifierStr);
						
							// Connect the newly created accelerator to the command
							accelerator.connectEvent(event);
						}
					} 
					else {
						globalOutputStream() << "EventManager: Warning: Cannot load shortcut definition (command invalid).\n";
					}
				}
			}
		}
		else {
			// No accelerator definitions found!
			globalOutputStream() << "EventManager: No shortcut definitions found...\n";
		}
	}
	
	void saveEventListToRegistry() {
		const std::string rootKey = "user/ui/input";
		
		// The visitor class to save each event definition into the registry
		// Note: the SaveEventVisitor automatically wipes all the existing shortcuts from the registry
		SaveEventVisitor visitor(rootKey, this);
		 
		foreachEvent(visitor);
	}
	
	void foreachEvent(IEventVisitor& eventVisitor) {
		// Cycle through the event and pass them to the visitor class
		for (EventMap::iterator i = _events.begin(); i != _events.end(); i++) {
			const std::string eventName = i->first;
			IEventPtr event = i->second;
			
			eventVisitor.visit(eventName, event);
		}
	}
	
	// Tries to locate an accelerator, that is connected to the given command
	IAccelerator& findAccelerator(const IEventPtr event) {
		// Cycle through the accelerators and check for matches
		for (AcceleratorList::iterator i = _accelerators.begin(); i != _accelerators.end(); i++) {
			if (i->match(event)) {
				// Return the reference to the found accelerator
				return (*i);
			}
		}

		// Return an empty accelerator if nothing is found
		return _emptyAccelerator;
	}

	// Returns a bit field with the according modifier flags set 
	std::string getModifierStr(const unsigned int& modifierFlags) {
		std::string returnValue = "";
		
		if ((modifierFlags & (1 << getModifierBitIndex("CONTROL"))) != 0) {
			returnValue += (returnValue != "") ? "+" : "";
			returnValue += "CONTROL";
		}
		
		if ((modifierFlags & (1 << getModifierBitIndex("SHIFT"))) != 0) {
			returnValue += (returnValue != "") ? "+" : "";
			returnValue += "SHIFT";
		}
		
		if ((modifierFlags & (1 << getModifierBitIndex("ALT"))) != 0) {
			returnValue += (returnValue != "") ? "+" : "";
			returnValue += "ALT";
		}
		
		return returnValue;
	}

private:

	AcceleratorList findAccelerator(const std::string& key, const std::string& modifierStr) {
		guint keyVal = getGDKCode(key);
		unsigned int modifierFlags = getModifierFlags(modifierStr); 
		
		return findAccelerator(keyVal, modifierFlags);
	}

	bool duplicateAccelerator(const std::string& key, const std::string& modifiers, IEventPtr event) {
		AcceleratorList accelList = findAccelerator(key, modifiers);
		
		for (AcceleratorList::iterator i = accelList.begin(); i != accelList.end(); i++) {
			// If one of the accelerators in the list matches the event, return true
			if (i->match(event)) {
				return true;
			}
		}
		
		return false;
	}

	AcceleratorList findAccelerator(const guint& keyVal, const unsigned int& modifierFlags) {
		AcceleratorList returnList;
		
		// Cycle through the accelerators and check for matches
		for (AcceleratorList::iterator i = _accelerators.begin(); i != _accelerators.end(); i++) {
			
			if (i->match(keyVal, modifierFlags)) {
				// Add the pointer to the found accelerators
				returnList.push_back((*i));
			}
		}

		return returnList;
	}

	// Returns the pointer to the accelerator for the given GdkEvent, but convert the key to uppercase before passing it
	AcceleratorList findAccelerator(GdkEventKey* event) {
		unsigned int keyval = gdk_keyval_to_upper(event->keyval);
		
		// greebo: I saw this in the original GTKRadiant code, maybe this is necessary to catch GTK_ISO_Left_Tab...
		if (keyval == GDK_ISO_Left_Tab) {
			keyval = GDK_Tab;
		}
		
		return findAccelerator(keyval, getKeyboardFlags(event->state));
	}

	// The GTK keypress callback
	static gboolean onKeyPress(GtkWindow* window, GdkEventKey* event, gpointer data) {
		// Convert the passed pointer onto a KeyEventManager pointer
		EventManager* self = reinterpret_cast<EventManager*>(data);
		
		// Try to find a matching accelerator
		AcceleratorList accelList = self->findAccelerator(event);
		
		if (accelList.size() > 0) {
			
			// Pass the execute() call to all found accelerators
			for (AcceleratorList::iterator i = accelList.begin(); i != accelList.end(); i++) {
				i->keyDown();
			}
			
			return true;
		}
		
		return false;
	}
	
	// The GTK keypress callback
	static gboolean onKeyRelease(GtkWindow* window, GdkEventKey* event, gpointer data) {
		// Convert the passed pointer onto a KeyEventManager pointer
		EventManager* self = reinterpret_cast<EventManager*>(data);
		
		// Try to find a matching accelerator
		AcceleratorList accelList = self->findAccelerator(event);
		
		if (accelList.size() > 0) {
			
			// Pass the execute() call to all found accelerators
			for (AcceleratorList::iterator i = accelList.begin(); i != accelList.end(); i++) {
				i->keyUp();
			}
			
			return true;
		}
		
		return false;
	}
	
	guint getGDKCode(const std::string& keyStr) {
		guint returnValue = gdk_keyval_to_upper(gdk_keyval_from_name(keyStr.c_str()));
		
		if (returnValue == GDK_VoidSymbol) {
			globalOutputStream() << "EventManager: Warning: Could not recognise key " << keyStr.c_str() << "\n";
		}

		return returnValue;
	} 
	
	void loadModifierDefinitions() {
		// Find all button definitions
		xml::NodeList modifierList = GlobalRegistry().findXPath("user/ui/input/modifiers//modifier");
		
		if (modifierList.size() > 0) {
			globalOutputStream() << "EventManager: Modifiers found: " << modifierList.size() << "\n";
			for (unsigned int i = 0; i < modifierList.size(); i++) {
				const std::string name = modifierList[i].getAttributeValue("name");
				
				int bitIndex;
				try {
					bitIndex = boost::lexical_cast<int>(modifierList[i].getAttributeValue("bitIndex"));
				}
				catch (boost::bad_lexical_cast e) {
					bitIndex = -1;
				}
				
				if (name != "" && bitIndex >= 0) {
					//std::cout << "EventMapper: Found modifier definition " << name.c_str() << " with BitIndex " << bitIndex << "\n";
					
					// Save the modifier ID into the map
					_modifierBitIndices[name] = static_cast<unsigned int>(bitIndex);
				} 
				else {
					globalOutputStream() << "EventManager: Warning: Invalid modifier definition found.\n";
				}
			}
		}
		else {
			// No Button definitions found!
			globalOutputStream() << "EventManager: Critical: No modifiers definitions found!\n";
		}
	}
	
	unsigned int getModifierFlags(const std::string& modifierStr) {
		StringParts parts;
		boost::algorithm::split(parts, modifierStr, boost::algorithm::is_any_of("+"));
		
		// Do we have any modifiers at all?
		if (parts.size() > 0) {
			unsigned int returnValue = 0;
			
			// Cycle through all the modifier names and construct the bitfield
			for (unsigned int i = 0; i < parts.size(); i++) {
				if (parts[i] == "") continue;
				
				// Try to find the modifierBitIndex
				int bitIndex = getModifierBitIndex(parts[i]);
							
				// Was anything found? 
				if (bitIndex >= 0) {
					unsigned int bitValue = (1 << static_cast<unsigned int>(bitIndex));
					returnValue |= bitValue;
				}
			}
			
			return returnValue;
		}
		else {
			return 0;
		}
	}
	
	GdkModifierType getGdkModifierType(const unsigned int& modifierFlags) {
		unsigned int returnValue = 0;
		
		if ((modifierFlags & (1 << getModifierBitIndex("CONTROL"))) != 0) {
			returnValue |= GDK_CONTROL_MASK;
		}
		
		if ((modifierFlags & (1 << getModifierBitIndex("SHIFT"))) != 0) {
			returnValue |= GDK_SHIFT_MASK;
		}
		
		if ((modifierFlags & (1 << getModifierBitIndex("ALT"))) != 0) {
			returnValue |= GDK_MOD1_MASK;
		}
		
		return static_cast<GdkModifierType>(returnValue); 
	}

	int getModifierBitIndex(const std::string& modifierName) {
		ModifierBitIndexMap::iterator it = _modifierBitIndices.find(modifierName);
	   	if (it != _modifierBitIndices.end()) {
	   		return it->second;
	   	}
	   	else {
	   		globalOutputStream() << "EventManager: Warning: Modifier " << modifierName.c_str() << " not found, returning -1\n";
	   		return -1;
	   	}
	}
	
	// Returns a bit field with the according modifier flags set 
	unsigned int getKeyboardFlags(const unsigned int& state) {
		unsigned int returnValue = 0;
		
		if ((state & GDK_CONTROL_MASK) != 0) {
	    	returnValue |= (1 << getModifierBitIndex("CONTROL"));
		}
		
		if ((state & GDK_SHIFT_MASK) != 0) {
	    	returnValue |= (1 << getModifierBitIndex("SHIFT"));
		}
		
		if ((state & GDK_MOD1_MASK) != 0) {
	    	returnValue |= (1 << getModifierBitIndex("ALT"));
		}
		
		return returnValue;
	}

}; // class EventManager


/* EventManager dependencies class. 
 */ 
class EventManagerDependencies :
	public GlobalRegistryModuleRef
{
};

/* Required code to register the module with the ModuleServer.
 */

#include "modulesystem/singletonmodule.h"

typedef SingletonModule<EventManager, EventManagerDependencies> EventManagerModule;

// Static instance of the EventManagerModule
EventManagerModule _theEventManagerModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server) {
	initialiseModule(server);
	_theEventManagerModule.selfRegister();
}
