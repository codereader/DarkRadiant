#ifndef _EVENT_MANAGER_H_
#define _EVENT_MANAGER_H_

#include "ieventmanager.h"

#include "gtk/gtkobject.h"
#include "gtk/gtkaccelgroup.h"

#include <map>
#include <vector>
#include <list>

#include "Accelerator.h"
#include "Modifiers.h"
#include "MouseEvents.h"

class EventManager : 
	public IEventManager
{
	// The handler ID of the connected keyboard handler
	typedef std::map<gulong, GtkObject*> HandlerMap;
	
	// Needed for boost::algorithm::split
	typedef std::vector<std::string> StringParts;

	// Each command has a name, this is the map where the name->command association is stored
	typedef std::map<const std::string, IEventPtr> EventMap;
	
	// The list of all allocated Accelerators
	typedef std::list<Accelerator> AcceleratorList;

	// The list of connect (top-level) windows, whose keypress events are immediately processed 
	HandlerMap _handlers;
	
	// The list of connected dialog window handlers, whose keypress events are 
	// processed AFTER the dialog window's default keyboard handler.
	HandlerMap _dialogWindows;
	
	// The list containing all registered accelerator objects
	AcceleratorList _accelerators;
	
	// The map of all registered events
	EventMap _events;
	
	// The GTK accelerator group for the main window
	GtkAccelGroup* _accelGroup;
	
	IEventPtr _emptyEvent;
	Accelerator _emptyAccelerator;
	
	Modifiers _modifiers;
	MouseEventManager _mouseEvents;
	
	bool _debugMode;
	
	// This stores the current keyboard state to allow client requests for modifiers
	GdkEventKey _eventKey;

public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();
	
	// Constructor
	EventManager();

	// Destructor, un-reference the GTK accelerator group
	~EventManager();
	
	void connectSelectionSystem(SelectionSystem* selectionSystem);
	
	// Returns a reference to the mouse event mapper
	IMouseEvents& MouseEvents();
	
	IAccelerator& addAccelerator(const std::string& key, const std::string& modifierStr);
	IAccelerator& addAccelerator(GdkEventKey* event);
	
	IEventPtr findEvent(const std::string& name);
	IEventPtr findEvent(GdkEventKey* event);
	
	std::string getEventName(const IEventPtr& event);
	
	std::string getAcceleratorStr(const IEventPtr& event, bool forMenu);
	
	// Checks if the eventName is already registered and writes to globalOutputStream, if so 
	bool alreadyRegistered(const std::string& eventName);
	
	// Add the given command to the internal list
	IEventPtr addCommand(const std::string& name, const Callback& callback, bool reactOnKeyUp);

	// Add a command and specify the statement to execute when triggered
	IEventPtr addCommand(const std::string& name, const std::string& statement, bool reactOnKeyUp);

	IEventPtr addKeyEvent(const std::string& name, const ui::KeyStateChangeCallback& keyStateChangeCallback);
	IEventPtr addWidgetToggle(const std::string& name);
	IEventPtr addRegistryToggle(const std::string& name, const std::string& registryKey);
	IEventPtr addToggle(const std::string& name, const Callback& onToggled);
	
	void setToggled(const std::string& name, const bool toggled);
	
	// Connects the given accelerator to the given command (identified by the string)
	void connectAccelerator(IAccelerator& accelerator, const std::string& command);
	void disconnectAccelerator(const std::string& command);
	
	void disableEvent(const std::string& eventName);
	void enableEvent(const std::string& eventName);

	void removeEvent(const std::string& eventName);

	// Catches the key/mouse press/release events from the given GtkObject
	void connect(GtkObject* object);	
	void disconnect(GtkObject* object);
	
	/* greebo: This connects an dialog window to the event handler. This means the following:
	 * 
	 * An incoming key-press event reaches the static method onDialogKeyPress which
	 * passes the key event to the connect dialog FIRST, before the key event has a
	 * chance to be processed by the standard shortcut processor. IF the dialog window
	 * standard handler returns TRUE, that is. If the gtk_window_propagate_key_event()
	 * function returns FALSE, the window couldn't find a use for this specific key event
	 * and the event can be passed safely to the onKeyPress() method. 
	 * 
	 * This way it is ensured that the dialog window can handle, say, text entries without
	 * firing global shortcuts all the time.   
	 */
	void connectDialogWindow(GtkWindow* window);
	
	void disconnectDialogWindow(GtkWindow* window);
	
	void connectAccelGroup(GtkWindow* window);
	
	// Loads the default shortcuts from the registry
	void loadAccelerators();
	
	void foreachEvent(IEventVisitor& eventVisitor);
	
	// Tries to locate an accelerator, that is connected to the given command
	IAccelerator& findAccelerator(const IEventPtr& event);

	// Returns the string representation of the given modifier flags 
	std::string getModifierStr(const unsigned int modifierFlags, bool forMenu = false);

	unsigned int getModifierState();

private:

	void saveEventListToRegistry();

	AcceleratorList findAccelerator(const std::string& key, const std::string& modifierStr);

	bool duplicateAccelerator(const std::string& key, const std::string& modifiers, const IEventPtr& event);

	AcceleratorList findAccelerator(guint keyVal, const unsigned int modifierFlags);

	// Returns the pointer to the accelerator for the given GdkEvent, but convert the key to uppercase before passing it
	AcceleratorList findAccelerator(GdkEventKey* event);

	// The GTK keypress callbacks for dialogs
	static gboolean onDialogKeyPress(GtkWindow* window, GdkEventKey* event, EventManager* self);
	static gboolean onDialogKeyRelease(GtkWindow* window, GdkEventKey* event, EventManager* self);
	
	void updateStatusText(GdkEventKey* event, bool keyPress);
	
	// The GTK keypress callbacks
	static gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data);	
	static gboolean onKeyRelease(GtkWidget* widget, GdkEventKey* event, gpointer data);
	
	guint getGDKCode(const std::string& keyStr);
	
	bool isModifier(GdkEventKey* event);	
	std::string getGDKEventStr(GdkEventKey* event);
};
typedef boost::shared_ptr<EventManager> EventManagerPtr;

#endif /* _EVENT_MANAGER_H_ */
