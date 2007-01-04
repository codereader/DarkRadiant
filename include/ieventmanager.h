#ifndef IEVENTMANAGER_H_
#define IEVENTMANAGER_H_

#include <list>
#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "generic/constant.h"
#include "generic/callbackfwd.h"

// GTK forward declaration
typedef struct _GtkObject GtkObject;
typedef struct _GtkWindow GtkWindow;
typedef struct _GtkWidget GtkWidget;

class IEvent
{
public:
	// Handles the incoming keyUp / keyDown calls
	virtual void keyUp() = 0;
	virtual void keyDown() = 0;
	
	// Enables/disables this event
	virtual void setEnabled(const bool enabled) = 0;
	
	// Connect a GtkWidget to this event (the event must support the according widget). 
	virtual void connectWidget(GtkWidget* widget) = 0; 
	
	// Exports the current state to the widgets
	virtual void updateWidgets() = 0;
	
	// Returns true if this event could be toggled (returns false if the event is not a Toggle).
	virtual bool setToggled(const bool toggled) = 0;
	
	// Returns true, if this is any empty Event (no command attached)
	virtual bool empty() const = 0;
};

typedef boost::shared_ptr<IEvent> IEventPtr;

class IAccelerator
{
public:
	// Get/set the GDK key value
	virtual void setKey(const unsigned int& key) = 0;
	virtual unsigned int getKey() const = 0;
	
	// Get/Set the modifier flags
	virtual void setModifiers(const unsigned int& modifiers) = 0;
	virtual unsigned int getModifiers() const = 0;
	
	// Connect this IEvent to this accelerator
	virtual void connectEvent(IEventPtr event) = 0; 
};

// Event visitor class
class IEventVisitor {
public:
	virtual void visit(const std::string& eventName, IEventPtr event) = 0;
};

class IEventManager
{
public:
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "EventManager");

	/* Create an accelerator using the given arguments and add it to the list
	 * 
	 * @key: The symbolic name of the key, e.g. "A", "Esc"
	 * @modifierStr: A string containing the modifiers, e.g. "Shift+Control" or "Shift"
	 * 
	 * @returns: the pointer to the newly created accelerator object */
	virtual IAccelerator& addAccelerator(const std::string& key, const std::string& modifierStr) = 0;
	virtual IAccelerator& findAccelerator(const IEventPtr event) = 0;
	
	// Creates a new command that calls the given callback when invoked  
	virtual IEventPtr addCommand(const std::string& name, const Callback& callback) = 0;
	
	// Creates a new keyevent that calls the given callback when invoked  
	virtual IEventPtr addKeyEvent(const std::string& name, const Callback& keyUpCallback, const Callback& keyDownCallback) = 0;
	
	// Creates a new toggle event that calls the given callback when toggled
	virtual IEventPtr addToggle(const std::string& name, const Callback& onToggled) = 0;
	virtual IEventPtr addWidgetToggle(const std::string& name) = 0;
	virtual IEventPtr addRegistryToggle(const std::string& name, const std::string& registryKey) = 0;
	
	// Set the according Toggle command (identified by <name>) to the bool <toggled> 
	virtual void setToggled(const std::string& name, const bool toggled) = 0;
	
	// Returns the pointer to the command specified by the <given> commandName
	virtual IEventPtr findEvent(const std::string& name) = 0;
	
	// Connects the given accelerator to the given command (identified by the string)
	virtual void connectAccelerator(IAccelerator& accelerator, const std::string& command) = 0;
	
	// Connects the keyboard handlers of the keyeventmanager to the specified window, so that key events are catched
	virtual void connect(GtkObject* object) = 0;
	
	// Tells the key event manager about the main window so that the accelgroup can be connected correctly
	virtual void connectAccelGroup(GtkWindow* window) = 0;
	
	// Loads the shortcut->command associations from the XMLRegistry
	virtual void loadAccelerators() = 0;
	
	// Saves the current event/accelerator state to the XMLRegistry (call this before exporting the registry)
	virtual void saveEventListToRegistry() = 0;
	
	// Enables/Disables the specified command
	virtual void enableEvent(const std::string& eventName) = 0;
	virtual void disableEvent(const std::string& eventName) = 0;
	
	// Visit each event with the given class
	virtual void foreachEvent(IEventVisitor& eventVisitor) = 0;
	
	virtual std::string getModifierStr(const unsigned int& modifierFlags) = 0;
};

// Module definitions

#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<IEventManager> GlobalEventManagerModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<IEventManager> GlobalEventManagerModuleRef;

// This is the accessor for the event manager
inline IEventManager& GlobalEventManager() {
	return GlobalEventManagerModule::getTable();
} 

#endif /*IEVENTMANAGER_H_*/
