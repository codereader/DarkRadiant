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
	_emptyAccelerator(0, 0, _emptyEvent)
{}

Accelerator& EventManager::addAccelerator(const std::string& key, const std::string& modifierStr)
{
	unsigned int keyVal = Accelerator::getKeyCodeFromName(key);
    unsigned int modifierFlags = wxutil::Modifier::GetStateFromModifierString(modifierStr);

	// Add a new Accelerator to the list
	_accelerators.emplace_back(std::make_shared<Accelerator>(keyVal, modifierFlags, _emptyEvent));

	// return the reference to the last accelerator in the list
	return *_accelerators.back();
}

Accelerator& EventManager::addAccelerator(wxKeyEvent& ev)
{
	int keyCode = ev.GetKeyCode();
	unsigned int modifierFlags = wxutil::Modifier::GetStateForKeyEvent(ev);

	// Create a new accelerator with the given arguments and add it
	_accelerators.emplace_back(std::make_shared<Accelerator>(keyCode, modifierFlags, _emptyEvent));

	// return the reference to the last accelerator in the list
	return *_accelerators.back();
}

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
	AcceleratorList accelList = findAccelerator(ev);

	// Did we find any matching accelerators? If yes, take the first found accelerator
	return !accelList.empty() ? (*accelList.begin())->getEvent() : _emptyEvent;
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
	_menuItems.emplace(eventName, item);

	// Set the accelerator of this menu item
	auto& accelerator = findAccelerator(eventName);
	
	item->setAccelerator(accelerator.getString(true));
}

void EventManager::unregisterMenuItem(const std::string& eventName, const ui::IMenuElementPtr& item)
{
	for (auto it = _menuItems.lower_bound(eventName); 
		 it != _menuItems.end() && it != _menuItems.upper_bound(eventName); ++it)
	{
		if (it->second == item)
		{
			_menuItems.erase(it);
			break;
		}
	}
}

// Connects the given accelerator to the given command (identified by the string)
void EventManager::connectAccelerator(IAccelerator& accelerator, const std::string& command)
{
	IEventPtr event = findEvent(command);

	if (!event->empty())
    {
		// Command found, connect it to the accelerator by passing its pointer
        event->connectAccelerator(accelerator);
        static_cast<Accelerator&>(accelerator).setEvent(event);
	}
	else
    {
		// Command NOT found
		rWarning() << "EventManager: Unable to connect command: " << command << std::endl;
	}
}

void EventManager::disconnectAccelerator(const std::string& command) 
{
	IEventPtr event = findEvent(command);

	if (!event->empty()) 
    {
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
	}
	else 
    {
		// Command NOT found
		rWarning() << "EventManager: Unable to disconnect command: " << command << std::endl;
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
	std::for_each(_events.begin(), _events.end(), [&] (EventMap::value_type& pair)
	{
		for (std::size_t tool = 0; tool < toolbar->GetToolsCount(); tool++)
		{
			pair.second->disconnectToolItem(const_cast<wxToolBarToolBase*>(toolbar->GetToolByPos(static_cast<int>(tool))));
		}
	});
}

// Loads the default shortcuts from the registry
void EventManager::loadAccelerators()
{
	// TODO
	//// Register all custom statements as events too to make them shortcut-bindable
	//// before going ahead
	//GlobalCommandSystem().foreachStatement([&](const std::string& statementName)
	//{
	//	addCommand(statementName, statementName, false);
	//}, true); // custom statements only

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

		// Try to lookup the command
		IEventPtr event = findEvent(cmd);

		// Check for valid command definitions were found
		if (!event->empty())
		{
			if (!duplicateAccelerator(key, modifierStr, event))
			{
				// Create the accelerator object
				Accelerator& accelerator = addAccelerator(key, modifierStr);

				// Connect the newly created accelerator to the command
                event->connectAccelerator(accelerator);
                accelerator.setEvent(event);
			}

			continue;
		}

#if 1
		// Second chance: look up a matching command
		if (GlobalCommandSystem().commandExists(cmd))
		{
			auto signature = GlobalCommandSystem().getSignature(cmd);

			if (signatureIsEmptyOrOptional(signature))
			{
				bool accelExists = !findAccelerator(key, modifierStr).empty();
				
				if (accelExists)
				{
					rWarning() << "EventManager: Shortcut is already taken: " 
						<< modifierStr << "+" << key << std::endl;
					continue;
				}

				Accelerator& accelerator = addAccelerator(key, modifierStr);

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
}

// Tries to locate an accelerator, that is connected to the given command
Accelerator& EventManager::findAccelerator(const IEventPtr& event)
{
	// Cycle through the accelerators and check for matches
    for (const auto& accel : _accelerators)
    {
		if (accel->match(event))
        {
			// Return the reference to the found accelerator
			return *accel;
		}
	}

	// Return an empty accelerator if nothing is found
	return _emptyAccelerator;
}

Accelerator& EventManager::findAccelerator(const std::string& commandName)
{
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
}

void EventManager::saveEventListToRegistry()
{
	const std::string rootKey = "user/ui/input";

	// The visitor class to save each event definition into the registry
	// Note: the SaveEventVisitor automatically wipes all the existing shortcuts from the registry
	SaveEventVisitor visitor(rootKey, *this);

	foreachEvent(visitor);
}

AcceleratorList EventManager::findAccelerator(
	const std::string& key, const std::string& modifierStr)
{
	unsigned int keyVal = Accelerator::getKeyCodeFromName(key);
    unsigned int modifierFlags = wxutil::Modifier::GetStateFromModifierString(modifierStr);

	return findAccelerator(keyVal, modifierFlags);
}

bool EventManager::duplicateAccelerator(const std::string& key,
										const std::string& modifiers,
										const IEventPtr& event)
{
	AcceleratorList accelList = findAccelerator(key, modifiers);

	for (const auto& accelerator : accelList)
    {
		// If one of the accelerators in the list matches the event, return true
		if (accelerator->match(event))
        {
			return true;
		}
	}

	return false;
}

AcceleratorList EventManager::findAccelerator(unsigned int keyVal,
                                              const unsigned int modifierFlags)
{
	AcceleratorList returnList;

	// Cycle through the accelerators and check for matches
	for (const auto& accelerator : _accelerators)
    {
		if (accelerator->match(keyVal, modifierFlags))
		{
			// Add the pointer to the found accelerators
			returnList.push_back(accelerator);
		}
	}

	return returnList;
}

AcceleratorList EventManager::findAccelerator(wxKeyEvent& ev)
{
	int keyval = ev.GetKeyCode(); // is always uppercase
	
	return findAccelerator(keyval, wxutil::Modifier::GetStateForKeyEvent(ev));
}

bool EventManager::handleKeyEvent(wxKeyEvent& keyEvent)
{
	AcceleratorList accelList = findAccelerator(keyEvent);

	// Pass the execute() call to all found accelerators
	for (const auto& accelerator : accelList)
	{
		const std::string& statement = accelerator->getStatement();

		if (!statement.empty())
		{
			if (keyEvent.GetEventType() == wxEVT_KEY_DOWN)
			{
				GlobalCommandSystem().execute(statement);
			}

			continue;
		}

		auto ev = accelerator->getEvent();

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
		}

		// Found an accelerator with no event or statement
	}

	return !accelList.empty(); // accelerators found => true
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
