#pragma once

#include <list>
#include <map>
#include <string>

#include <memory>

#include "imodule.h"
#include <functional>

class wxWindow;
class wxMenuItem;
class wxToolBarToolBase;
class wxButton;
class wxToggleButton;
class wxMouseEvent;
class wxKeyEvent;
class wxToolBar;
class wxTopLevelWindow;

/* greebo: Below are the actual events that are "read" by the views/observers to
 * interpret the mouseclicks. */

namespace ui 
{

	// Enum used for events tracking the key state
	enum KeyEventType
	{
		KeyPressed,
		KeyReleased,
	};
	typedef std::function<void (KeyEventType)> KeyStateChangeCallback;

	class IMenuElement;
	typedef std::shared_ptr<IMenuElement> IMenuElementPtr;

} // namespace ui

class IAccelerator
{
public:
    // destructor
    virtual ~IAccelerator() {}

    // Get/set the key value
    virtual void setKey(const int key) = 0;
    virtual int getKey() const = 0;

    // Get/Set the modifier flags
    virtual void setModifiers(const unsigned int modifiers) = 0;
    virtual unsigned int getModifiers() const = 0;

	// Returns a string representation of this accelerator.
	// forMenu == true: returns the localised modifier strings
	// and is using a different separator to prevent wxWidgets 
	// from assigning accelerators on its own.
	virtual std::string getString(bool forMenu) const = 0;
};

class IEvent
{
public:
    // destructor
	virtual ~IEvent() {}

	// Handles the incoming keyUp / keyDown calls
	virtual void keyUp() = 0;
	virtual void keyDown() = 0;

	// Enables/disables this event
	virtual void setEnabled(const bool enabled) = 0;

	// Connect a wxTopLevelWindow to this event
	virtual void connectTopLevelWindow(wxTopLevelWindow* widget) = 0;
	virtual void disconnectTopLevelWindow(wxTopLevelWindow* widget) = 0;

	virtual void connectToolItem(wxToolBarToolBase* item) = 0;
	virtual void disconnectToolItem(wxToolBarToolBase* item) = 0;

	virtual void connectMenuItem(wxMenuItem* item) = 0;
	virtual void disconnectMenuItem(wxMenuItem* item) = 0;

	virtual void connectButton(wxButton* button) = 0;
	virtual void disconnectButton(wxButton* button) = 0;

	virtual void connectToggleButton(wxToggleButton* button) = 0;
	virtual void disconnectToggleButton(wxToggleButton* button) = 0;

	virtual void connectMenuItem(const ui::IMenuElementPtr& item) = 0;
	virtual void disconnectMenuItem(const ui::IMenuElementPtr& item) = 0;

	// Exports the current state to the widgets
	virtual void updateWidgets() = 0;

	// Returns true if this event could be toggled (returns false if the event is not a Toggle).
	virtual bool setToggled(const bool toggled) = 0;

	/** greebo: Returns true if the event is a Toggle (or a subtype of a Toggle)
	 */
	virtual bool isToggle() const = 0;

	// Returns true, if this is any empty Event (no command attached)
	virtual bool empty() const = 0;

    // Associate an accelerator to this event (this can trigger an update of the associated widgets)
    virtual void connectAccelerator(IAccelerator& accel) = 0;

    // Signal to remove the accelerators from this event (might update associated widgets)
    virtual void disconnectAccelerators() = 0;
};
typedef std::shared_ptr<IEvent> IEventPtr;

// Event visitor class
class IEventVisitor
{
public:
    // destructor
	virtual ~IEventVisitor() {}

	virtual void visit(const std::string& eventName, const IAccelerator& accel) = 0;
};

const char* const MODULE_EVENTMANAGER("EventManager");

// The function object invoked when a ToggleEvent is changing states
// The passed boolean indicates the new toggle state (true = active/toggled)
typedef std::function<void(bool)> ToggleCallback;

class IEventManager :
	public RegisterableModule
{
public:
	// The same as above, but with event values as argument (event->keyval, event->state)
	virtual std::string getAcceleratorStr(const IEventPtr& event, bool forMenu) = 0;

	// Loads all accelerator bindings from the defaults in the stock input.xml
	virtual void resetAcceleratorBindings() = 0;

	// Add a command and specify the statement to execute when triggered
	virtual IEventPtr addCommand(const std::string& name, const std::string& statement, bool reactOnKeyUp) = 0;

	// Creates a new keyevent that calls the given callback when invoked
	virtual IEventPtr addKeyEvent(const std::string& name, const ui::KeyStateChangeCallback& keyStateChangeCallback) = 0;

	// Creates a new toggle event that calls the given callback when toggled
	virtual IEventPtr addToggle(const std::string& name, const ToggleCallback& onToggled) = 0;
	virtual IEventPtr addWidgetToggle(const std::string& name) = 0;
	virtual IEventPtr addRegistryToggle(const std::string& name, const std::string& registryKey) = 0;

	// Set the according Toggle command (identified by <name>) to the bool <toggled>
	virtual void setToggled(const std::string& name, const bool toggled) = 0;

	// Returns the pointer to the command specified by the <given> commandName
	virtual IEventPtr findEvent(const std::string& name) = 0;
	virtual IEventPtr findEvent(wxKeyEvent& ev) = 0;

	// Retrieves the event name for the given IEventPtr
	virtual std::string getEventName(const IEventPtr& event) = 0;

	// Connects the given accelerator to the given command (identified by the string)
	virtual void connectAccelerator(wxKeyEvent& keyEvent, const std::string& command) = 0;
	// Disconnects the given command from any accelerators
	virtual void disconnectAccelerator(const std::string& command) = 0;

	// Before destruction, it's advisable to disconnect any events from a toolbar's items
	virtual void disconnectToolbar(wxToolBar* toolbar) = 0;

	// Register the given menu item with the given command. The event manager updates this item
	// when the accelerator association changes
	virtual void registerMenuItem(const std::string& eventName, const ui::IMenuElementPtr& item) = 0;
	virtual void unregisterMenuItem(const std::string& eventName, const ui::IMenuElementPtr& item) = 0;

	virtual void registerToolItem(const std::string& eventName, wxToolBarToolBase* item) = 0;

	// Loads the shortcut->command associations from the XMLRegistry
	virtual void loadAccelerators() = 0;

	// Enables/Disables the specified command
	virtual void enableEvent(const std::string& eventName) = 0;
	virtual void disableEvent(const std::string& eventName) = 0;

	// Renames the given command, keeping accelerator bindings intact
	virtual void renameEvent(const std::string& oldEventName, const std::string& newEventName) = 0;

	// Removes the given event and disconnects all accelerators from it
	virtual void removeEvent(const std::string& eventName) = 0;

	// Visit each event with the given class
	virtual void foreachEvent(IEventVisitor& eventVisitor) = 0;

	/* greebo: Retrieves the string representation of the given event
	 */
	virtual std::string getEventStr(wxKeyEvent& ev) = 0;
};
typedef std::shared_ptr<IEventManager> IEventManagerPtr;

// This is the accessor for the event manager
inline IEventManager& GlobalEventManager() {
	// Cache the reference locally
	static IEventManager& _eventManager(
		*std::static_pointer_cast<IEventManager>(
			module::GlobalModuleRegistry().getModule(MODULE_EVENTMANAGER)
		)
	);
	return _eventManager;
}
