#include "ieventmanager.h"

#include "iregistry.h"
#include <iostream>

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

class EventManager : 
	public IEventManager 
{
	// The handler ID of the connected keyboard handler
	typedef std::list<gulong> HandlerList;
	
	// Needed for boost::algorithm::split
	typedef std::vector<std::string> StringParts;

	typedef std::map<const std::string, unsigned int> ModifierBitIndexMap;
	
	// Each command has a name, this is the map where the name->command association is stored
	typedef std::map<const std::string, IEvent*> EventMap;
	
	// The list of all allocated Accelerators
	typedef std::vector<Accelerator*> AcceleratorList;
		
	HandlerList _handlers;
	
	// The list containing all registered accelerator objects
	AcceleratorList _accelerators;
	
	// The map of all registered events
	EventMap _events;
	
	// The list of all modifier bit indices
	ModifierBitIndexMap _modifierBitIndices;
	
	// The GTK accelerator group for the main window
	GtkAccelGroup* _accelGroup;

public:
	// Radiant Module stuff
	typedef IEventManager Type;
	STRING_CONSTANT(Name, "*");

	// Return the static instance
	IEventManager* getTable() {
		return this;
	}
	
	// Constructor
	EventManager() {
		globalOutputStream() << "EventManager started.\n";
		_accelGroup = gtk_accel_group_new();
		
		loadModifierDefinitions();
	}
	
	// Destructor, free all allocated objects
	~EventManager() {
		// Remove all accelerators from the heap
		for (AcceleratorList::iterator i = _accelerators.begin(); i != _accelerators.end(); i++) {
			Accelerator* accelerator = (*i);
			delete accelerator;
		}
		_accelerators.clear();
		
		// Remove all commands from the heap
		for (EventMap::iterator i = _events.begin(); i != _events.end(); i++) {
			IEvent* event = i->second;
			delete event;
		}
		_events.clear();
		
		globalOutputStream() << "EventManager successfully shut down.\n";
	}
	
	IAccelerator* addAccelerator(const std::string& key, const std::string& modifierStr) {
		guint keyVal = getGDKCode(key);
		unsigned int modifierFlags = getModifierFlags(modifierStr); 
		
		// Allocate a new accelerator object on the heap
		Accelerator* accelerator = new Accelerator(keyVal, modifierFlags);
		
		_accelerators.push_back(accelerator);
		
		// return the pointer to the new accelerator
		return accelerator;
	}
	
	IEvent* findEvent(const std::string& name) {
		// Try to lookup the command
		EventMap::iterator i = _events.find(name);
		
		if (i != _events.end()) {
			// Return the pointer to the command
			return i->second;
		}
		else {
			// Nothing found, return NULL
			return NULL;
		}
	}
	
	// Add the given command to the internal list
	IEvent* addCommand(const std::string& name, const Callback& callback) {
		
		// Try to find the command and see if it's already registered
		IEvent* foundEvent = findEvent(name);
		 
		if (foundEvent == NULL) {
			// Construct a new command with the given callback
			Command* cmd = new Command(callback);
			
			// Add the command to the list (implicitly cast the pointer on IEvent*)  
			_events[name] = cmd;
			
			// Return the pointer to the newly created event
			return cmd;
		}
		else {
			globalOutputStream() << "EventManager: Warning: Event " << name.c_str() << " already registered!\n";
			return foundEvent;
		}
	}
	
	IEvent* addKeyEvent(const std::string& name, const Callback& keyUpCallback, const Callback& keyDownCallback) {
		// Try to find the command and see if it's already registered
		IEvent* foundEvent = findEvent(name);
		 
		if (foundEvent == NULL) {
			// Construct a new keyevent with the given callback
			KeyEvent* keyevent = new KeyEvent(keyUpCallback, keyDownCallback);
			
			// Add the command to the list (implicitly cast the pointer on IEvent*)  
			_events[name] = keyevent;
			
			// Return the pointer to the newly created event
			return keyevent;
		}
		else {
			globalOutputStream() << "EventManager: Warning: Event " << name.c_str() << " already registered!\n";
			return foundEvent;
		}
	}
	
	IEvent* addWidgetToggle(const std::string& name) {
		// Try to find the command and see if it's already registered
		IEvent* foundEvent = findEvent(name);
		
		if (foundEvent == NULL) {
			// Construct a new command with the given <onToggled> callback
			WidgetToggle* widgetToggle = new WidgetToggle();
			
			// Add the command to the list (implicitly cast the pointer on IEvent*)  
			_events[name] = widgetToggle;
			
			// Return the pointer to the newly created event
			return widgetToggle;
		}
		else {
			globalOutputStream() << "EventManager: Warning: Event " << name.c_str() << " already registered!\n";
			return foundEvent;
		}
	}
	
	IEvent* addRegistryToggle(const std::string& name, const std::string& registryKey) {
		// Try to find the command and see if it's already registered
		IEvent* foundEvent = findEvent(name);
		
		if (foundEvent == NULL) {
			// Construct a new command with the given <onToggled> callback
			RegistryToggle* registryToggle = new RegistryToggle(registryKey);
			
			// Add the command to the list (implicitly cast the pointer on IEvent*)  
			_events[name] = registryToggle;
			
			// Return the pointer to the newly created event
			return registryToggle;
		}
		else {
			globalOutputStream() << "EventManager: Warning: Event " << name.c_str() << " already registered!\n";
			return foundEvent;
		}
	}
	
	IEvent* addToggle(const std::string& name, const Callback& onToggled) {
		// Try to find the command and see if it's already registered
		IEvent* foundEvent = findEvent(name);
		 
		if (foundEvent == NULL) {
			// Construct a new command with the given <onToggled> callback
			Toggle* toggle = new Toggle(onToggled);
			
			// Add the command to the list (implicitly cast the pointer on IEvent*)  
			_events[name] = toggle;
			
			// Return the pointer to the newly created event
			return toggle;
		}
		else {
			globalOutputStream() << "EventManager: Warning: Event " << name.c_str() << " already registered!\n";
			return foundEvent;
		}
	}
	
	void setToggled(const std::string& name, const bool toggled) {
		// Try to find the command and see if it's already registered
		Toggle* foundToggle = dynamic_cast<Toggle*>(findEvent(name));
		 
		if (foundToggle != NULL) {
			foundToggle->setToggled(toggled);
		}
	}
	
	// Connects the given accelerator to the given command (identified by the string)
	void connectAccelerator(IAccelerator* accelerator, const std::string& command) {
		// Sanity check
		if (accelerator != NULL) {
			IEvent* event = findEvent(command);
			
			if (event != NULL) {
				// Command found, connect it to the accelerator by passing its pointer			
				accelerator->connectEvent(event);
			}
			else {
				// Command NOT found
				globalOutputStream() << "EventManager: Unable to lookup command: " << command.c_str() << "\n";
			}
		}
	}
	
	void disableEvent(const std::string& eventName) {
		IEvent* event = findEvent(eventName);
		
		if (event != NULL) {
			event->setEnabled(false);
		}
	}
	
	void enableEvent(const std::string& eventName) {
		IEvent* event = findEvent(eventName);
		
		if (event != NULL) {
			event->setEnabled(true);
		}
	}

	// Catches the keypress/keyrelease events from the given GtkObject
	void connect(GtkObject* object)	{
		_handlers.push_back(g_signal_connect(G_OBJECT(object), "key_press_event", G_CALLBACK(onKeyPress), this));
		_handlers.push_back(g_signal_connect(G_OBJECT(object), "key_release_event", G_CALLBACK(onKeyRelease), this));
	}
	
	void connectAccelGroup(GtkWindow* window) {
		gtk_window_add_accel_group(window, _accelGroup); 
	}
	
	// Loads the default shortcuts from the registry
	void loadAccelerators() {
		// Find all accelerators
		xml::NodeList shortcutList = GlobalRegistry().findXPath("user/ui/input/shortcuts//shortcut");
		
		if (shortcutList.size() > 0) {
			globalOutputStream() << "EventManager: Shortcuts found in Registry: " << shortcutList.size() << "\n";
			for (unsigned int i = 0; i < shortcutList.size(); i++) {
				const std::string key = shortcutList[i].getAttributeValue("key");
				
				// Try to lookup the command
				IEvent* event = findEvent(shortcutList[i].getAttributeValue("command"));
				
				// Check if valid key / command definitions were found
				if (key != "" && event != NULL) {
					// Get the modifier string (e.g. "SHIFT+ALT")
					const std::string modifierStr = shortcutList[i].getAttributeValue("modifiers");
					
					if (!duplicateAccelerator(key, modifierStr, event)) {
						// Create the accelerator object
						IAccelerator* accelerator = addAccelerator(key, modifierStr);
					
						// Connect the newly created accelerator to the command
						accelerator->connectEvent(event);
					}
				} 
				else {
					globalOutputStream() << "EventManager: Warning: Cannot load shortcut definition (key/command invalid).\n";
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
		
		foreachEvent(&visitor);
	}
	
	void foreachEvent(IEventVisitor* eventVisitor) {
		// Cycle through the event and pass them to the visitor class
		for (EventMap::iterator i = _events.begin(); i != _events.end(); i++) {
			const std::string eventName = i->first;
			IEvent* event = i->second;
			
			eventVisitor->visit(eventName, event);
		}
	}
	
	// Tries to locate an accelerator, that is connected to the given command
	IAccelerator* findAccelerator(const IEvent* event) {
		// Cycle through the accelerators and check for matches
		for (unsigned int i = 0; i < _accelerators.size(); i++) {
			if (_accelerators[i]->match(event)) {
				// Return the pointer to the found accelerator
				return _accelerators[i];
			}
		}

		return NULL;
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

	bool duplicateAccelerator(const std::string& key, const std::string& modifiers, IEvent* event) {
		AcceleratorList list = findAccelerator(key, modifiers);
		
		for (unsigned int i = 0; i < list.size(); i++) {
			// If one of the accelerators in the list matches the event, return true
			if (list[i]->match(event)) {
				return true;
			}
		}
		
		return false;
	}

	AcceleratorList findAccelerator(const guint& keyVal, const unsigned int& modifierFlags) {
		AcceleratorList returnList;
		
		// Cycle through the accelerators and check for matches
		for (unsigned int i = 0; i < _accelerators.size(); i++) {
			
			if (_accelerators[i]->match(keyVal, modifierFlags)) {
				// Add the pointer to the found accelerators
				returnList.push_back(_accelerators[i]);
			}
		}

		return returnList;
	}

	AcceleratorList findAccelerator(const std::string& key, const std::string& modifierStr) {
		guint keyVal = getGDKCode(key);
		unsigned int modifierFlags = getModifierFlags(modifierStr); 
		
		return findAccelerator(keyVal, modifierFlags);
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
			for (unsigned int i = 0; i < accelList.size(); i++) {
				Accelerator* accelerator = dynamic_cast<Accelerator*>(accelList[i]);
				
				if (accelerator != NULL) {
					// A matching accelerator has been found, pass the keyDown event
					accelerator->keyDown();
				}
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
			for (unsigned int i = 0; i < accelList.size(); i++) {
				Accelerator* accelerator = dynamic_cast<Accelerator*>(accelList[i]);
				
				if (accelerator != NULL) {
					// A matching accelerator has been found, pass the keyDown event
					accelerator->keyUp();
				}
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
