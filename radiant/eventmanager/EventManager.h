#pragma once

#include "ieventmanager.h"
#include <wx/event.h>

#include <map>
#include <list>

#include "xmlutil/Node.h"
#include "Accelerator.h"

#include "GlobalKeyEventFilter.h"

namespace ui
{

class EventManager :
	public IEventManager,
	public wxEvtHandler
{
private:
	// Each command has a name, this is the map where the name->command association is stored
	typedef std::map<const std::string, IEventPtr> EventMap;

	// The list containing all registered accelerator objects
	AcceleratorList _accelerators;

	// The map of all registered events
	EventMap _events;

	IEventPtr _emptyEvent;
	Accelerator _emptyAccelerator;

    GlobalKeyEventFilterPtr _shortcutFilter;

public:
	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

	// Constructor
	EventManager();

	Accelerator& addAccelerator(const std::string& key, const std::string& modifierStr) override;
	Accelerator& addAccelerator(wxKeyEvent& ev) override;

	IEventPtr findEvent(const std::string& name) override;
	IEventPtr findEvent(wxKeyEvent& ev) override;

	std::string getEventName(const IEventPtr& event) override;

	std::string getAcceleratorStr(const IEventPtr& event, bool forMenu) override;

	void resetAcceleratorBindings() override;

	// Checks if the eventName is already registered and writes to rMessage, if so
	bool alreadyRegistered(const std::string& eventName);

	// Add a command and specify the statement to execute when triggered
	IEventPtr addCommand(const std::string& name, const std::string& statement, bool reactOnKeyUp) override;

	IEventPtr addKeyEvent(const std::string& name, const KeyStateChangeCallback& keyStateChangeCallback) override;
	IEventPtr addWidgetToggle(const std::string& name) override;
	IEventPtr addRegistryToggle(const std::string& name, const std::string& registryKey) override;
	IEventPtr addToggle(const std::string& name, const ToggleCallback& onToggled) override;

	void setToggled(const std::string& name, const bool toggled) override;

	// Connects the given accelerator to the given command (identified by the string)
	void connectAccelerator(IAccelerator& accelerator, const std::string& command) override;
	void disconnectAccelerator(const std::string& command) override;

	void disableEvent(const std::string& eventName) override;
	void enableEvent(const std::string& eventName) override;

	void removeEvent(const std::string& eventName) override;

	void disconnectToolbar(wxToolBar* toolbar) override;

	// Loads the default shortcuts from the registry
	void loadAccelerators() override;

	void foreachEvent(IEventVisitor& eventVisitor) override;

	// Tries to locate an accelerator, that is connected to the given command
	Accelerator& findAccelerator(const IEventPtr& event) override;
    AcceleratorList findAccelerator(wxKeyEvent& ev);

	std::string getEventStr(wxKeyEvent& ev) override;

private:
	void saveEventListToRegistry();

	AcceleratorList findAccelerator(const std::string& key, const std::string& modifierStr);

	bool duplicateAccelerator(const std::string& key, const std::string& modifiers, const IEventPtr& event);

	// Returns the pointer to the accelerator for the given event, but convert the key to uppercase before passing it
	AcceleratorList findAccelerator(unsigned int keyVal, const unsigned int modifierFlags);

	void loadAcceleratorFromList(const xml::NodeList& shortcutList);

	bool isModifier(wxKeyEvent& ev);
};

}
