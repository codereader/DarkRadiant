#pragma once

#include "ui/ieventmanager.h"
#include <wx/event.h>
#include <sigc++/connection.h>

#include <map>
#include <list>

#include "xmlutil/Node.h"
#include "Accelerator.h"

#include "GlobalKeyEventFilter.h"

namespace ui
{

class EventManager :
	public wxEvtHandler,
	public IEventManager
{
private:
	// Each command has a name, this is the map where the name->command association is stored
	typedef std::map<const std::string, IEventPtr> EventMap;

	std::multimap<std::string, wxMenuItem*> _menuItems;
	std::multimap<std::string, const wxToolBarToolBase*> _toolItems;

	// Reverse mapping of menu IDs back to command strings
	std::map<int, std::string> _commandsByMenuID;

	// The command-to-accelerator map containing all registered shortcuts
	typedef std::map<std::string, Accelerator::Ptr> AcceleratorMap;
	AcceleratorMap _accelerators;

	// The map of all registered events
	EventMap _events;

	IEventPtr _emptyEvent;
	Accelerator _emptyAccelerator;

    GlobalKeyEventFilterPtr _shortcutFilter;

public:
	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

	// Constructor
	EventManager();

	IEventPtr findEvent(const std::string& name) override;
	std::string findEventForAccelerator(wxKeyEvent& ev) override;

	std::string getEventName(const IEventPtr& event) override;

	void resetAcceleratorBindings() override;

	// Checks if the eventName is already registered and writes to rMessage, if so
	bool alreadyRegistered(const std::string& eventName);

	// Add a command and specify the statement to execute when triggered
	IEventPtr addCommand(const std::string& name, const std::string& statement, bool reactOnKeyUp) override;

	IEventPtr addKeyEvent(const std::string& name, const KeyStateChangeCallback& keyStateChangeCallback) override;
	IEventPtr addWidgetToggle(const std::string& name) override;
	IEventPtr addRegistryToggle(const std::string& name, const std::string& registryKey) override;
	IEventPtr addAdvancedToggle(const std::string& name, const AdvancedToggleCallback& onToggled) override;

	void setToggled(const std::string& name, const bool toggled) override;

	void registerMenuItem(const std::string& eventName, wxMenuItem* item) override;
	void unregisterMenuItem(const std::string& eventName, wxMenuItem* item) override;

	void registerToolItem(const std::string& eventName, const wxToolBarToolBase* item) override;
	void unregisterToolItem(const std::string& eventName, const wxToolBarToolBase* item) override;

	// Connects the given accelerator to the given command (identified by the string)
	void connectAccelerator(wxKeyEvent& keyEvent, const std::string& command) override;
	void disconnectAccelerator(const std::string& command) override;

	void disableEvent(const std::string& eventName) override;
	void enableEvent(const std::string& eventName) override;

	void renameEvent(const std::string& oldEventName, const std::string& newEventName) override;
	void removeEvent(const std::string& eventName) override;

	// Loads the default shortcuts from the registry
	void loadAccelerators() override;

	void foreachEvent(IEventVisitor& eventVisitor) override;

	// Tries to locate an accelerator, that is connected to the given command
	Accelerator& findAccelerator(wxKeyEvent& ev);
	bool handleKeyEvent(wxKeyEvent& keyEvent);

	std::string getEventStr(wxKeyEvent& ev) override;

    IAccelerator::Ptr findAcceleratorForEvent(const std::string& eventName) override;

private:
	void saveEventListToRegistry();

	Accelerator& connectAccelerator(int keyCode, unsigned int modifierFlags, const std::string& command);

	Accelerator& findAccelerator(const std::string& commandName);
	Accelerator& findAccelerator(const std::string& key, const std::string& modifierStr);

	// Returns the iterator to the mapping for the given key comob (convert the key to uppercase before passing it)
	AcceleratorMap::iterator findAccelerator(unsigned int keyVal, const unsigned int modifierFlags);

	void loadAcceleratorFromList(const xml::NodeList& shortcutList);

	void setMenuItemAccelerator(const std::string& command, const std::string& acceleratorStr);
	void setToolItemAccelerator(const std::string& command, const std::string& acceleratorStr);

	bool isModifier(wxKeyEvent& ev);
	void onToolItemClicked(wxCommandEvent& ev);
	void onMenuItemClicked(wxCommandEvent& ev);
    void aboutToOpenMenu(wxMenu& ev);
};

}
