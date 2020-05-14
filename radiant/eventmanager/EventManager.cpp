#include "EventManager.h"

#include "imodule.h"
#include "iradiant.h"
#include "itextstream.h"
#include "icommandsystem.h"
#include "iselection.h"
#include <iostream>
#include <typeinfo>

#include <wx/wxprec.h>
#include <wx/toolbar.h>

#include "registry/registry.h"
#include "xmlutil/Node.h"
#include "wxutil/Modifier.h"

#include "Statement.h"
#include "Toggle.h"
#include "WidgetToggle.h"
#include "RegistryToggle.h"
#include "KeyEvent.h"
#include "SaveEventVisitor.h"
#include "MouseToolManager.h"

#include "debugging/debugging.h"
#include "module/StaticModule.h"
#include <iostream>

namespace ui
{

namespace
{

inline bool signatureIsEmptyOrOptional(const cmd::Signature& signature)
{
	for (auto part : signature)
	{
		if ((part & cmd::ARGTYPE_OPTIONAL) == 0)
		{
			return false;
		}
	}

	return true; // empty or is fully optional
}

}

// RegisterableModule implementation
const std::string& EventManager::getName() const
{
	static std::string _name(MODULE_EVENTMANAGER);
	return _name;
}

const StringSet& EventManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
	}

	return _dependencies;
}

void EventManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// Deactivate the empty event, so it's safe to return it as NullEvent
	_emptyEvent->setEnabled(false);

    _shortcutFilter.reset(new GlobalKeyEventFilter(*this));

	rMessage() << getName() << " successfully initialised." << std::endl;
}

void EventManager::shutdownModule()
{
	rMessage() << getName() << "::shutdownModule called" << std::endl;
    _shortcutFilter.reset();

	saveEventListToRegistry();

	_accelerators.clear();
	_events.clear();
}

// Constructor
EventManager::EventManager() :
	_emptyEvent(new Event()),
	_emptyAccelerator(Accelerator::CreateEmpty())
{}

#if 0
Accelerator& EventManager::addAccelerator(const std::string& key, const std::string& modifierStr)
{
	unsigned int keyVal = Accelerator::getKeyCodeFromName(key);
    unsigned int modifierFlags = wxutil::Modifier::GetStateFromModifierString(modifierStr);

	// Add a new Accelerator to the list
	_accelerators.emplace_back(std::make_shared<Accelerator>(keyVal, modifierFlags));

	// return the reference to the last accelerator in the list
	return *_accelerators.back();
}
#endif

#if 0
Accelerator& EventManager::addAccelerator(wxKeyEvent& ev)
{
	int keyCode = ev.GetKeyCode();
	unsigned int modifierFlags = wxutil::Modifier::GetStateForKeyEvent(ev);

	// Create a new accelerator with the given arguments and add it
	_accelerators.emplace_back(std::make_shared<Accelerator>(keyCode, modifierFlags, _emptyEvent));

	// return the reference to the last accelerator in the list
	return *_accelerators.back();
}
#endif

void EventManager::resetAcceleratorBindings()
{
	// Select the stock mappings
	std::string xPathQuery = "user/ui/input/shortcuts[@name='default']//shortcut";

	xml::NodeList shortcutList = GlobalRegistry().findXPath(xPathQuery);

	if (shortcutList.empty())
	{
		// No accelerator definitions found!
		rWarning() << "EventManager: No default shortcut definitions found..." << std::endl;
		return;
	}

	// Disconnect all accelerators from all events
	for (EventMap::value_type& pair : _events)
	{
		pair.second->disconnectAccelerators();
	}

	_accelerators.clear();

	for (const auto& pair : _menuItems)
	{
		pair.second.item->setAccelerator(std::string());
	}

	rMessage() << "EventManager: Default shortcuts found in Registry: " << shortcutList.size() << std::endl;

	loadAcceleratorFromList(shortcutList);
}

IEventPtr EventManager::findEvent(const std::string& name) 
{
	// Try to lookup the command
	auto found = _events.find(name);

#if 0
	if (found == _events.end())
	{
		// Try to look up a matching command in the CommandSystem
		if (GlobalCommandSystem().commandExists(name))
		{
			auto signature = GlobalCommandSystem().getSignature(name);

			if (signatureIsEmptyOrOptional(signature))
			{
				// Insert a new event wrapping this command
				found = _events.emplace(name, std::make_shared<Statement>(name)).first;
			}
		}
	}
#endif

	// if nothing found, return the NullEvent
	return found != _events.end() ? found->second : _emptyEvent;
}

IEventPtr EventManager::findEvent(wxKeyEvent& ev)
{
	// Retrieve the accelerators for this eventkey
	Accelerator& accel = findAccelerator(ev);

	// Did we find any matching accelerators? If yes, take the first found accelerator
	return accel.getEvent() ? accel.getEvent() : _emptyEvent;
}

std::string EventManager::getEventName(const IEventPtr& event)
{
	// Try to lookup the given eventptr
	for (const EventMap::value_type& pair : _events)
	{
		if (pair.second == event)
		{
			return pair.first;
		}
	}

	return std::string();
}

std::string EventManager::getAcceleratorStr(const IEventPtr& event, bool forMenu)
{
	IAccelerator& accelerator = findAccelerator(event);

    return accelerator.getString(forMenu);
}

// Checks if the eventName is already registered and writes to rWarning, if so
bool EventManager::alreadyRegistered(const std::string& eventName)
{
	// Try to find the command and see if it's already registered
	IEventPtr foundEvent = findEvent(eventName);

	if (foundEvent->empty())
	{
		return false;
	}
	
	rWarning() << "EventManager: Event " << eventName << " already registered!" << std::endl;
	return true;
}

// Add the given command to the internal list
IEventPtr EventManager::addCommand(const std::string& name, const std::string& statement, bool reactOnKeyUp)
{
	if (alreadyRegistered(name))
	{
		return _emptyEvent;
	}

	// Add the command to the list
	IEventPtr event = std::make_shared<Statement>(statement, reactOnKeyUp);

	_events[name] = event;

	// Return the pointer to the newly created event
	return event;
}

IEventPtr EventManager::addKeyEvent(const std::string& name, const KeyStateChangeCallback& keyStateChangeCallback)
{
	if (alreadyRegistered(name))
	{
		return _emptyEvent;
	}
	
	IEventPtr event = std::make_shared<KeyEvent>(keyStateChangeCallback);

	// Add the new keyevent to the list
	_events[name] = event;

	// Return the pointer to the newly created event
	return event;
}

IEventPtr EventManager::addWidgetToggle(const std::string& name) {

	if (alreadyRegistered(name))
	{
		return _emptyEvent;
	}
	
	IEventPtr event = std::make_shared<WidgetToggle>();

	// Add the command to the list
	_events[name] = event;

	// Return the pointer to the newly created event
	return event;
}

IEventPtr EventManager::addRegistryToggle(const std::string& name, const std::string& registryKey)
{
	if (alreadyRegistered(name))
	{
		return _emptyEvent;
	}

	IEventPtr event = std::make_shared<RegistryToggle>(registryKey);
		
	// Add the command to the list 
	_events[name] = event;
	
	// Return the pointer to the newly created event
	return event;
}

IEventPtr EventManager::addToggle(const std::string& name, const ToggleCallback& onToggled)
{
	if (alreadyRegistered(name))
	{
		return _emptyEvent;
	}

	IEventPtr event = std::make_shared<Toggle>(onToggled);

	// Add the command to the list
	_events[name] = event;

	// Return the pointer to the newly created event
	return event;
}

void EventManager::setToggled(const std::string& name, const bool toggled)
{
	// Check could be placed here by std::dynamic_pointer_cast
	if (!findEvent(name)->setToggled(toggled))
	{
		rWarning() << "EventManager: Event " << name << " is not a Toggle." << std::endl;
	}
}

void EventManager::registerMenuItem(const std::string& eventName, const ui::IMenuElementPtr& item)
{
	auto result = _menuItems.emplace(eventName, ItemConnection());

	result->second.item = item;

	// Set the accelerator of this menu item
	auto& accelerator = findAccelerator(eventName);
	
	item->setAccelerator(accelerator.getString(true));

	// Check if we have an event object corresponding to this event name
	auto evt = findEvent(eventName);

	if (!evt->empty())
	{
		evt->connectMenuItem(item);
	}
	else
	{
		// There's no special event object this item is connected to
		// just wire up an execute() when this item is clicked
		result->second.itemActivatedConn = item->signal_ItemActivated().connect(
			[=]() { GlobalCommandSystem().execute(eventName); }
		);
	}
}

void EventManager::unregisterMenuItem(const std::string& eventName, const ui::IMenuElementPtr& item)
{
	for (auto it = _menuItems.lower_bound(eventName); 
		 it != _menuItems.end() && it != _menuItems.upper_bound(eventName); ++it)
	{
		if (it->second.item == item)
		{
			it->second.itemActivatedConn.disconnect();
			_menuItems.erase(it);
			break;
		}
	}

	// Check if we have an event object corresponding to this event name
	auto evt = findEvent(eventName);

	if (!evt->empty())
	{
		evt->disconnectMenuItem(item);
	}
}

void EventManager::registerToolItem(const std::string& eventName, wxToolBarToolBase* item)
{
	auto result = _toolItems.emplace(eventName, item);

	// Set the accelerator of this menu item
	auto& accelerator = findAccelerator(eventName);

	Event::setToolItemAccelerator(item, accelerator);

	// Check if we have an event object corresponding to this event name
	auto evt = findEvent(eventName);

	if (!evt->empty())
	{
		evt->connectToolItem(item);
	}
	else
	{
		// There's no special event object this item is connected to
		// just wire up an execute() when this item is clicked
		item->GetToolBar()->Bind(wxEVT_TOOL, &EventManager::onToolItemClicked, this, item->GetId());
	}
}

void EventManager::onToolItemClicked(wxCommandEvent& ev)
{
	for (const auto & pair : _toolItems)
	{
		if (pair.second->GetId() == ev.GetId())
		{
			GlobalCommandSystem().execute(pair.first);
			break;
		}
	}
}

Accelerator& EventManager::connectAccelerator(int keyCode, unsigned int modifierFlags, const std::string& command)
{
	auto result = _accelerators.emplace(command, std::make_shared<Accelerator>(keyCode, modifierFlags));
	Accelerator& accel = *result.first->second;

	// There might be an event to associate
	auto event = findEvent(command);

	if (!event->empty())
	{
		accel.setEvent(event);
	}
	else
	{
		accel.setStatement(command);
	}

	std::string acceleratorStr = accel.getString(true);
	setMenuItemAccelerator(command, acceleratorStr);
	setToolItemAccelerator(command, acceleratorStr);

	return *result.first->second;
#if 0
	else
	{
	// Command NOT found
	rWarning() << "EventManager: Unable to connect command: " << command << std::endl;
	}
#endif
}

// Connects the given accelerator to the given command (identified by the string)
void EventManager::connectAccelerator(wxKeyEvent& keyEvent, const std::string& command)
{
	int keyCode = keyEvent.GetKeyCode();
	unsigned int modifierFlags = wxutil::Modifier::GetStateForKeyEvent(keyEvent);

	connectAccelerator(keyCode, modifierFlags, command);
}

void EventManager::disconnectAccelerator(const std::string& command) 
{
	auto existing = _accelerators.find(command);

	if (existing != _accelerators.end())
	{
		// Clear menu item accelerator string
		setMenuItemAccelerator(command, std::string());

		if (existing->second->getEvent())
		{
			existing->second->getEvent()->disconnectAccelerators();
			existing->second->setEvent(_emptyEvent);
		}

		_accelerators.erase(existing);
	}

#if 0
	// Cycle through the accelerators and check for matches
	for (const auto& accel : _accelerators)
	{
		if (accel->match(event))
		{
			// Connect the accelerator to the empty event (disable the accelerator)
			event->disconnectAccelerators();

			accel->setEvent(_emptyEvent);
			accel->setKey(0);
			accel->setModifiers(0);
		}
	}

	IEventPtr event = findEvent(command);

	if (!event->empty()) 
    {
		
	}
	else 
    {
		// Command NOT found
		rWarning() << "EventManager: Unable to disconnect command: " << command << std::endl;
	}
#endif
}

void EventManager::setToolItemAccelerator(const std::string& command, const std::string& acceleratorStr)
{
	for (auto it = _toolItems.lower_bound(command); 
		 it != _toolItems.end() && it != _toolItems.upper_bound(command); ++it)
	{
		Event::setToolItemAccelerator(it->second, acceleratorStr);
	}
}

void EventManager::setMenuItemAccelerator(const std::string& command, const std::string& acceleratorStr)
{
	for (auto it = _menuItems.lower_bound(command); 
		 it != _menuItems.end() && it != _menuItems.upper_bound(command); ++it)
	{
		it->second.item->setAccelerator(acceleratorStr);
	}
}

void EventManager::disableEvent(const std::string& eventName) 
{
	findEvent(eventName)->setEnabled(false);
}

void EventManager::enableEvent(const std::string& eventName) 
{
	findEvent(eventName)->setEnabled(true);
}

void EventManager::renameEvent(const std::string& oldEventName, const std::string& newEventName)
{
	auto existing = _events.find(oldEventName);

	if (existing == _events.end())
	{
		rError() << "Cannot rename event, this name is not registered: " << oldEventName << std::endl;
	}

	// Re-insert the event using a new name
	auto existingEvent = existing->second;
	_events.erase(existing);
	_events.insert(std::make_pair(newEventName, existingEvent));
}

void EventManager::removeEvent(const std::string& eventName) 
{
	// Try to lookup the command
	EventMap::iterator i = _events.find(eventName);

	if (i != _events.end()) 
	{
		// Remove all accelerators beforehand
		disconnectAccelerator(eventName);

		// Remove the event from the list
		_events.erase(i);
	}
}

void EventManager::disconnectToolbar(wxToolBar* toolbar)
{
	for (std::size_t tool = 0; tool < toolbar->GetToolsCount(); tool++)
	{
		for (auto it = _toolItems.begin(); it != _toolItems.end(); ++it)
		{
			auto toolItem = toolbar->GetToolByPos(tool);

			if (it->second == toolItem)
			{
				auto evt = findEvent(it->first);

				if (!evt->empty())
				{
					evt->disconnectToolItem(toolItem);
				}
				else
				{
					toolbar->Unbind(wxEVT_TOOL, &EventManager::onToolItemClicked, this, toolItem->GetId());
				}

				_toolItems.erase(it);
				break;
			}
		}
	}

	for (EventMap::value_type& pair : _events)
	{
		for (std::size_t tool = 0; tool < toolbar->GetToolsCount(); tool++)
		{
			pair.second->disconnectToolItem(const_cast<wxToolBarToolBase*>(toolbar->GetToolByPos(static_cast<int>(tool))));
		}
	}
}

// Loads the default shortcuts from the registry
void EventManager::loadAccelerators()
{
	xml::NodeList shortcutSets = GlobalRegistry().findXPath("user/ui/input//shortcuts");

	// If we have two sets of shortcuts, don't select the stock ones
	std::string xPathQuery = shortcutSets.size() > 1 ?
		"user/ui/input/shortcuts[not(@name)]//shortcut" : // find all without name attribute
		"user/ui/input/shortcuts//shortcut"; // find all shortcuts

	xml::NodeList shortcutList = GlobalRegistry().findXPath(xPathQuery);

	if (shortcutList.empty())
	{
		// No accelerator definitions found!
		rWarning() << "EventManager: No shortcut definitions found..." << std::endl;
		return;
	}

	rMessage() << "EventManager: Shortcuts found in Registry: " << shortcutList.size() << std::endl;

	loadAcceleratorFromList(shortcutList);
}

void EventManager::loadAcceleratorFromList(const xml::NodeList& shortcutList)
{
	for (const xml::Node& shortcutNode : shortcutList)
	{
		const std::string key = shortcutNode.getAttributeValue("key");
		const std::string cmd = shortcutNode.getAttributeValue("command");
		// Get the modifier string (e.g. "SHIFT+ALT")
		const std::string modifierStr = shortcutNode.getAttributeValue("modifiers");

		// Check for a non-empty key string
		if (key.empty()) continue;

		// Check for duplicate accelerator mappings
		const auto& existingAccel = findAccelerator(key, modifierStr);

		if (existingAccel.getStatement() == cmd)
		{
			rWarning() << "Accelerator " << existingAccel.getString(false) <<
				" is already mapped to " << cmd << std::endl;
			continue;
		}

		// Create the accelerator object
		int keyVal = Accelerator::getKeyCodeFromName(key);
		unsigned int modifierFlags = wxutil::Modifier::GetStateFromModifierString(modifierStr);

#if 0
		auto& accelerator = 
#endif
		connectAccelerator(keyVal, modifierFlags, cmd);

#if 0
		// Update registered menu item
		setMenuItemAccelerator(cmd, accelerator.getString(true));

		// Try to lookup the command
		IEventPtr event = findEvent(cmd);

		// Check for valid command definitions
		if (!event->empty())
		{
			// Connect the newly created accelerator to the command
            event->connectAccelerator(accelerator);
            accelerator.setEvent(event);
			continue;
		}
#endif

#if 0
		// Second chance: look up a matching command
		if (GlobalCommandSystem().commandExists(cmd))
		{
			auto signature = GlobalCommandSystem().getSignature(cmd);

			if (signatureIsEmptyOrOptional(signature))
			{
				accelerator.setStatement(cmd);
				continue;
			}
		}
#endif

		rWarning() << "EventManager: Cannot load shortcut definition (command invalid): " 
			<< cmd << std::endl;
	}
}

void EventManager::foreachEvent(IEventVisitor& eventVisitor)
{
	// Cycle through the event and pass them to the visitor class
	for (const auto& pair : _events)
	{
		auto& accel = findAccelerator(pair.second);
		eventVisitor.visit(pair.first, accel);
	}

	// Visit all commands without signatures
	GlobalCommandSystem().foreachCommand([&](const std::string& command)
	{
		auto signature = GlobalCommandSystem().getSignature(command);

		if (signatureIsEmptyOrOptional(signature))
		{
			auto accel = _accelerators.find(command);

			eventVisitor.visit(command, 
				accel != _accelerators.end() ? *accel->second : _emptyAccelerator);
		}
	});
}

// Tries to locate an accelerator, that is connected to the given command
Accelerator& EventManager::findAccelerator(const IEventPtr& event)
{
	// Cycle through the accelerators and check for matches
    for (const auto& pair : _accelerators)
    {
		if (pair.second->match(event))
        {
			// Return the reference to the found accelerator
			return *pair.second;
		}
	}

	// Return an empty accelerator if nothing is found
	return _emptyAccelerator;
}

Accelerator& EventManager::findAccelerator(const std::string& commandName)
{
	auto found = _accelerators.find(commandName);

	if (found != _accelerators.end())
	{
		return *found->second;
	}

	return _emptyAccelerator;
#if 0
	auto foundEvent = _events.find(commandName);

	// Cycle through the accelerators and check for matches
    for (const auto& accel : _accelerators)
    {
		if (accel->getStatement() == commandName)
        {
			// Return the reference to the found accelerator
			return *accel;
		}

		// If we got an event wrapper, check if this accelerator is associated to it
		if (foundEvent != _events.end() && accel->match(foundEvent->second))
		{
			return *accel;
		}
	}

	// Return an empty accelerator if nothing is found
	return _emptyAccelerator;
#endif
}

void EventManager::saveEventListToRegistry()
{
	// The visitor class to save each event definition into the registry
	// Note: the SaveEventVisitor automatically wipes all the existing shortcuts from the registry
	SaveEventVisitor shortcutSaver("user/ui/input");

	// We don't use the foreachEvent() iterator here since the CommandSystem
	// might already have cleared out all registered commands
	for (const auto& pair : _events)
	{
		auto& accel = findAccelerator(pair.second);
		shortcutSaver.visit(pair.first, accel);
	}

	for (const auto& pair : _accelerators)
	{
		if (pair.second->isEmpty()) continue;

		shortcutSaver.visit(pair.first, *pair.second);
	}
}

Accelerator& EventManager::findAccelerator(const std::string& key, const std::string& modifierStr)
{
	unsigned int keyVal = Accelerator::getKeyCodeFromName(key);
    unsigned int modifierFlags = wxutil::Modifier::GetStateFromModifierString(modifierStr);

	return findAccelerator(keyVal, modifierFlags);
}

bool EventManager::duplicateAccelerator(const std::string& key,
										const std::string& modifiers,
										const IEventPtr& event)
{
#if 0
	Accelerator& accelerator = findAccelerator(key, modifiers);

	for (const auto& accelerator : accelList)
    {
		// If one of the accelerators in the list matches the event, return true
		if (accelerator->match(event))
        {
			return true;
		}
	}
#endif
	return false;
}

Accelerator& EventManager::findAccelerator(unsigned int keyVal, unsigned int modifierFlags)
{
	// Cycle through the accelerators and check for matches
	for (const auto& pair : _accelerators)
    {
		if (pair.second->match(keyVal, modifierFlags))
		{
			// Add the pointer to the found accelerators
			return *pair.second;
		}
	}

	return _emptyAccelerator;
}

Accelerator& EventManager::findAccelerator(wxKeyEvent& ev)
{
	int keyval = ev.GetKeyCode(); // is always uppercase
	
	return findAccelerator(keyval, wxutil::Modifier::GetStateForKeyEvent(ev));
}

bool EventManager::handleKeyEvent(wxKeyEvent& keyEvent)
{
	Accelerator& accelerator = findAccelerator(keyEvent);

	const std::string& statement = accelerator.getStatement();

	if (!statement.empty())
	{
		if (keyEvent.GetEventType() == wxEVT_KEY_DOWN)
		{
			GlobalCommandSystem().execute(statement);
		}

		return true;
	}

	auto ev = accelerator.getEvent();

	if (ev)
	{
		if (keyEvent.GetEventType() == wxEVT_KEY_DOWN)
		{
			ev->keyDown();
		}
		else
		{
			ev->keyUp();
		}

		return true;
	}

	return false;
}

bool EventManager::isModifier(wxKeyEvent& ev)
{
	int keyCode = ev.GetKeyCode();

	return (keyCode == WXK_SHIFT || keyCode == WXK_CONTROL ||
		keyCode == WXK_ALT || keyCode == WXK_WINDOWS_LEFT ||
		keyCode == WXK_WINDOWS_MENU || keyCode == WXK_WINDOWS_RIGHT);
}

namespace
{

std::string getKeyString(wxKeyEvent& ev)
{
	int keycode = ev.GetKeyCode();
	std::string virtualKeyCodeName = Accelerator::getNameFromKeyCode(keycode);

	if (!virtualKeyCodeName.empty())
	{
        return virtualKeyCodeName;
	}

    if (keycode > 0 && keycode < 32)
	{
        return wxString::Format("Ctrl-%c", (unsigned char)('A' + keycode - 1)).ToStdString();
	}

    if (keycode >= 32 && keycode < 128)
	{
		return wxString::Format("%c", (unsigned char)keycode).ToStdString();
	}

	return "unknown";
}

} // namespace

std::string EventManager::getEventStr(wxKeyEvent& ev)
{
	std::string returnValue("");

	// Don't react on modifiers only (no actual key like A, 2, U, etc.)
	if (isModifier(ev))
	{
		return returnValue;
	}

	// Convert the key event to modifier flags
	const unsigned int modifierFlags = wxutil::Modifier::GetStateForKeyEvent(ev);

	// Construct the complete string
    returnValue += wxutil::Modifier::GetLocalisedModifierString(modifierFlags);
	returnValue += (returnValue != "") ? "-" : "";

	returnValue += getKeyString(ev);

	return returnValue;
}

// Static module instances
module::StaticModule<EventManager> eventManagerModule;
module::StaticModule<MouseToolManager> mouseToolManagerModule;

}
