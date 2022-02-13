#include "EventManager.h"

#include "imodule.h"
#include "itextstream.h"
#include "icommandsystem.h"
#include <iostream>
#include <typeinfo>

#include <wx/wxprec.h>
#include <wx/toolbar.h>
#include <wx/menu.h>

#include "registry/registry.h"
#include "xmlutil/Node.h"
#include "wxutil/Modifier.h"

#include "Statement.h"
#include "Toggle.h"
#include "WidgetToggle.h"
#include "RegistryToggle.h"
#include "KeyEvent.h"
#include "ShortcutSaver.h"

#include "module/StaticModule.h"

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
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void EventManager::initialiseModule(const IApplicationContext& ctx)
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

	_accelerators.clear();

	for (const auto& pair : _menuItems)
	{
		Event::setMenuItemAccelerator(pair.second, std::string());
	}

	rMessage() << "EventManager: Default shortcuts found in Registry: " << shortcutList.size() << std::endl;

	loadAcceleratorFromList(shortcutList);
}

IEventPtr EventManager::findEvent(const std::string& name)
{
	// Try to lookup the command
	auto found = _events.find(name);

	// if nothing found, return the NullEvent
	return found != _events.end() ? found->second : _emptyEvent;
}

std::string EventManager::findEventForAccelerator(wxKeyEvent& ev)
{
	int keyval = ev.GetKeyCode(); // is always uppercase

	auto it = findAccelerator(keyval, wxutil::Modifier::GetStateForKeyEvent(ev));

	// Did we find any matching accelerators? If yes, take the first found accelerator
	return it != _accelerators.end() ? it->first : std::string();
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

IEventPtr EventManager::addAdvancedToggle(const std::string& name,
                                          const AdvancedToggleCallback& onToggled)
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

void EventManager::registerMenuItem(const std::string& eventName, wxMenuItem* item)
{
	_menuItems.emplace(eventName, item);

	// Set the accelerator of this menu item
	auto& accelerator = findAccelerator(eventName);

	Event::setMenuItemAccelerator(item, accelerator);

	// Check if we have an event object corresponding to this event name
	auto evt = findEvent(eventName);

	if (!evt->empty())
	{
		evt->connectMenuItem(item);
	}
	else
	{
		item->GetMenu()->Bind(wxEVT_MENU, &EventManager::onMenuItemClicked, this, item->GetId());
	}
}

void EventManager::unregisterMenuItem(const std::string& eventName, wxMenuItem* item)
{
	for (auto it = _menuItems.lower_bound(eventName);
		 it != _menuItems.end() && it != _menuItems.upper_bound(eventName); ++it)
	{
		if (it->second != item) continue;

		// Check if we have an event object corresponding to this event name
		auto evt = findEvent(eventName);

		if (!evt->empty())
		{
			evt->disconnectMenuItem(item);
		}
		else
		{
			item->GetMenu()->Unbind(wxEVT_MENU, &EventManager::onMenuItemClicked, this, item->GetId());
		}

		_menuItems.erase(it);
		break;
	}
}

void EventManager::registerToolItem(const std::string& eventName, const wxToolBarToolBase* item)
{
	_toolItems.emplace(eventName, item);

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

void EventManager::unregisterToolItem(const std::string& eventName, const wxToolBarToolBase* item)
{
	for (auto it = _toolItems.begin(); it != _toolItems.end(); ++it)
	{
		if (it->second == item)
		{
			auto evt = findEvent(it->first);

			if (!evt->empty())
			{
				evt->disconnectToolItem(item);
			}
			else
			{
				item->GetToolBar()->Unbind(wxEVT_TOOL, &EventManager::onToolItemClicked, this, item->GetId());
			}

			_toolItems.erase(it);
			break;
		}
	}

	for (EventMap::value_type& pair : _events)
	{
		pair.second->disconnectToolItem(item);
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

void EventManager::onMenuItemClicked(wxCommandEvent& ev)
{
	for (const auto& pair : _menuItems)
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
	auto accelerator = std::make_shared<Accelerator>(keyCode, modifierFlags);

	// There might be an event to associate
	auto event = findEvent(command);

	if (!event->empty())
	{
		accelerator->setEvent(event);
	}
	else if (GlobalCommandSystem().commandExists(command))
	{
		accelerator->setStatement(command);
	}
	else
	{
		return _emptyAccelerator;
	}

	auto result = _accelerators.emplace(command, accelerator);

	std::string acceleratorStr = accelerator->getString(true);
	setMenuItemAccelerator(command, acceleratorStr);
	setToolItemAccelerator(command, acceleratorStr);

	return *result.first->second;
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
		// Clear menu item and tool item accelerator strings
		setMenuItemAccelerator(command, std::string());
		setToolItemAccelerator(command, std::string());

		if (existing->second->getEvent())
		{
			existing->second->setEvent(_emptyEvent);
		}

		_accelerators.erase(existing);
	}
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
		Event::setMenuItemAccelerator(it->second, acceleratorStr);
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
	auto found = _events.find(eventName);

	if (found != _events.end())
	{
		// Remove all accelerators beforehand
		disconnectAccelerator(eventName);

		// Remove the event from the list
		_events.erase(found);
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
	// Load command remappings to migrate legacy shortcuts
	std::map<std::string, std::string> commandRemap;

	auto remaps = GlobalRegistry().findXPath("user/ui/input//commandRemapping");

	for (const auto& node : remaps)
	{
		commandRemap.emplace(node.getAttributeValue("oldname"), node.getAttributeValue("newname"));
	}

	for (const xml::Node& shortcutNode : shortcutList)
	{
		const std::string key = shortcutNode.getAttributeValue("key");
		// Get the modifier string (e.g. "SHIFT+ALT")
		const std::string modifierStr = shortcutNode.getAttributeValue("modifiers");

		std::string cmd = shortcutNode.getAttributeValue("command");

		auto replacement = commandRemap.find(cmd);

		if (replacement != commandRemap.end())
		{
			rMessage() << "Mapping shortcut of legacy command " << replacement->first <<
				" to " << replacement->second << std::endl;
			cmd = replacement->second;
		}

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

		if (connectAccelerator(keyVal, modifierFlags, cmd).isEmpty())
		{
			rWarning() << "EventManager: Cannot load shortcut definition (command invalid): "
				<< cmd << std::endl;
		}
	}
}

void EventManager::foreachEvent(IEventVisitor& eventVisitor)
{
	// Cycle through all events and pass them to the visitor class
	for (const auto& pair : _events)
	{
		auto accel = _accelerators.find(pair.first);
		eventVisitor.visit(pair.first,
			accel != _accelerators.end() ? *accel->second : _emptyAccelerator);
	}

	// Visit all commands without mandatory parameters
	GlobalCommandSystem().foreachCommand([&](const std::string& command)
	{
		if (_events.find(command) != _events.end())
		{
			return; // skip any commands that have been covered in the event loop above
		}

		auto signature = GlobalCommandSystem().getSignature(command);

		if (signatureIsEmptyOrOptional(signature))
		{
			auto accel = _accelerators.find(command);
			eventVisitor.visit(command,
				accel != _accelerators.end() ? *accel->second : _emptyAccelerator);
		}
	});
}

Accelerator& EventManager::findAccelerator(const std::string& commandName)
{
	auto found = _accelerators.find(commandName);

	if (found != _accelerators.end())
	{
		return *found->second;
	}

	return _emptyAccelerator;
}

void EventManager::saveEventListToRegistry()
{
	// The visitor class to save each event definition into the registry
	// Note: the SaveEventVisitor automatically wipes all the existing shortcuts from the registry
	ShortcutSaver shortcutSaver("user/ui/input");

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

	auto it = findAccelerator(keyVal, modifierFlags);

	return it != _accelerators.end() ? *it->second : _emptyAccelerator;
}

EventManager::AcceleratorMap::iterator EventManager::findAccelerator(unsigned int keyVal, unsigned int modifierFlags)
{
	// Cycle through the accelerators and check for matches
	for (AcceleratorMap::iterator it = _accelerators.begin(); it != _accelerators.end(); ++it)
    {
		if (it->second->match(keyVal, modifierFlags))
		{
			return it;
		}
	}

	return _accelerators.end();
}

Accelerator& EventManager::findAccelerator(wxKeyEvent& ev)
{
	int keyval = ev.GetKeyCode(); // is always uppercase

	auto it = findAccelerator(keyval, wxutil::Modifier::GetStateForKeyEvent(ev));

	return it != _accelerators.end() ? *it->second : _emptyAccelerator;
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

// Static module registration
module::StaticModuleRegistration<EventManager> eventManagerModule;

}
