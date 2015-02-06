#pragma once

#include "ieventmanager.h"
#include <wx/event.h>

#include <map>
#include <vector>
#include <list>

#include "Accelerator.h"

#include "GlobalKeyEventFilter.h"

#include <sigc++/connection.h>

namespace ui
{

class EventManager :
	public IEventManager,
	public wxEvtHandler
{
private:
	// Needed for boost::algorithm::split
	typedef std::vector<std::string> StringParts;

	// Each command has a name, this is the map where the name->command association is stored
	typedef std::map<const std::string, IEventPtr> EventMap;

	// The list containing all registered accelerator objects
	AcceleratorList _accelerators;

	// The map of all registered events
	EventMap _events;

	IEventPtr _emptyEvent;
	Accelerator _emptyAccelerator;

	bool _debugMode;

    ui::GlobalKeyEventFilterPtr _shortcutFilter;

public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

	// Constructor
	EventManager();

	IAccelerator& addAccelerator(const std::string& key, const std::string& modifierStr);
	IAccelerator& addAccelerator(wxKeyEvent& ev);

	IEventPtr findEvent(const std::string& name);
	IEventPtr findEvent(wxKeyEvent& ev);

	std::string getEventName(const IEventPtr& event);

	std::string getAcceleratorStr(const IEventPtr& event, bool forMenu);

	// Checks if the eventName is already registered and writes to rMessage, if so
	bool alreadyRegistered(const std::string& eventName);

	// Add a command and specify the statement to execute when triggered
	IEventPtr addCommand(const std::string& name, const std::string& statement, bool reactOnKeyUp);

	IEventPtr addKeyEvent(const std::string& name, const ui::KeyStateChangeCallback& keyStateChangeCallback);
	IEventPtr addWidgetToggle(const std::string& name);
	IEventPtr addRegistryToggle(const std::string& name, const std::string& registryKey);
	IEventPtr addToggle(const std::string& name, const ToggleCallback& onToggled);

	void setToggled(const std::string& name, const bool toggled);

	// Connects the given accelerator to the given command (identified by the string)
	void connectAccelerator(IAccelerator& accelerator, const std::string& command);
	void disconnectAccelerator(const std::string& command);

	void disableEvent(const std::string& eventName);
	void enableEvent(const std::string& eventName);

	void removeEvent(const std::string& eventName);

	void disconnectToolbar(wxToolBar* toolbar);

	// Loads the default shortcuts from the registry
	void loadAccelerators();

	void foreachEvent(IEventVisitor& eventVisitor);

	// Tries to locate an accelerator, that is connected to the given command
	IAccelerator& findAccelerator(const IEventPtr& event);
    AcceleratorList findAccelerator(wxKeyEvent& ev);

	std::string getEventStr(wxKeyEvent& ev);

private:

	void saveEventListToRegistry();

	AcceleratorList findAccelerator(const std::string& key, const std::string& modifierStr);

	bool duplicateAccelerator(const std::string& key, const std::string& modifiers, const IEventPtr& event);

	// Returns the pointer to the accelerator for the given event, but convert the key to uppercase before passing it
	AcceleratorList findAccelerator(unsigned int keyVal, const unsigned int modifierFlags);

	bool isModifier(wxKeyEvent& ev);
};

}
