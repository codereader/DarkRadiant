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
#include <iostream>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>

namespace ui
{

	namespace
    {
		const std::string RKEY_DEBUG = "debug/ui/debugEventManager";
	}

// RegisterableModule implementation
const std::string& EventManager::getName() const {
	static std::string _name(MODULE_EVENTMANAGER);
	return _name;
}

const StringSet& EventManager::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
	}

	return _dependencies;
}

void EventManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "EventManager::initialiseModule called." << std::endl;

	_debugMode = registry::getValue<bool>(RKEY_DEBUG);

	// Deactivate the empty event, so it's safe to return it as NullEvent
	_emptyEvent->setEnabled(false);

    _shortcutFilter.reset(new ui::GlobalKeyEventFilter(*this));

	if (_debugMode)
    {
		rMessage() << "EventManager intitialised in debug mode." << std::endl;
	}
	else 
    {
		rMessage() << "EventManager successfully initialised." << std::endl;
	}
}

void EventManager::shutdownModule()
{
	rMessage() << "EventManager: shutting down." << std::endl;
    _shortcutFilter.reset();

	saveEventListToRegistry();

	_accelerators.clear();
	_events.clear();
}

// Constructor
EventManager::EventManager() :
	_emptyEvent(new Event()),
	_emptyAccelerator(0, 0, _emptyEvent),
	_debugMode(false)
{}

IAccelerator& EventManager::addAccelerator(const std::string& key, const std::string& modifierStr)
{
	unsigned int keyVal = Accelerator::getKeyCodeFromName(key);
    unsigned int modifierFlags = wxutil::Modifier::GetStateFromModifierString(modifierStr);

	Accelerator accel(keyVal, modifierFlags, _emptyEvent);

	// Add a new Accelerator to the list
	_accelerators.push_back(accel);

	// return the reference to the last accelerator in the list
	AcceleratorList::reverse_iterator i = _accelerators.rbegin();

	return (*i);
}

IAccelerator& EventManager::addAccelerator(wxKeyEvent& ev)
{
	int keyCode = ev.GetKeyCode();
	unsigned int modifierFlags = wxutil::Modifier::GetStateForKeyEvent(ev);

	// Create a new accelerator with the given arguments
	Accelerator accel(keyCode, modifierFlags, _emptyEvent);

	// Add a new Accelerator to the list
	_accelerators.push_back(accel);

	// return the reference to the last accelerator in the list
	return *_accelerators.rbegin();
}

IEventPtr EventManager::findEvent(const std::string& name) {
	// Try to lookup the command
	EventMap::iterator i = _events.find(name);

	if (i != _events.end()) {
		// Return the pointer to the command
		return i->second;
	}
	else {
		// Nothing found, return the NullEvent
		return _emptyEvent;
	}
}

IEventPtr EventManager::findEvent(wxKeyEvent& ev)
{
	// Retrieve the accelerators for this eventkey
	AcceleratorList accelList = findAccelerator(ev);

	// Did we find any matching accelerators? If yes, take the first found accelerator
	return !accelList.empty() ? accelList.begin()->getEvent() : _emptyEvent;
}

std::string EventManager::getEventName(const IEventPtr& event)
{
	// Try to lookup the given eventptr
	for (EventMap::iterator i = _events.begin(); i != _events.end(); i++) {
		if (i->second == event) {
			return i->first;
		}
	}

	return "";
}

std::string EventManager::getAcceleratorStr(const IEventPtr& event, bool forMenu)
{
	std::string returnValue = "";

	IAccelerator& accelerator = findAccelerator(event);

    return static_cast<Accelerator&>(accelerator).getAcceleratorString(forMenu);
}

// Checks if the eventName is already registered and writes to rMessage, if so
bool EventManager::alreadyRegistered(const std::string& eventName) {
	// Try to find the command and see if it's already registered
	IEventPtr foundEvent = findEvent(eventName);

	if (foundEvent->empty()) {
		return false;
	}
	else {
		rWarning() << "EventManager: Event " << eventName
			<< " already registered!" << std::endl;
		return true;
	}
}

// Add the given command to the internal list
IEventPtr EventManager::addCommand(const std::string& name, const std::string& statement, bool reactOnKeyUp) {

	if (!alreadyRegistered(name)) {
		// Add the command to the list
		_events[name] = IEventPtr(new Statement(statement, reactOnKeyUp));

		// Return the pointer to the newly created event
		return _events[name];
	}

	return _emptyEvent;
}

IEventPtr EventManager::addKeyEvent(const std::string& name, const ui::KeyStateChangeCallback& keyStateChangeCallback)
{
	if (!alreadyRegistered(name))
	{
		// Add the new keyevent to the list (implicitly cast onto Event&)
		_events[name] = IEventPtr(new KeyEvent(keyStateChangeCallback));

		// Return the pointer to the newly created event
		return _events[name];
	}

	return _emptyEvent;
}

IEventPtr EventManager::addWidgetToggle(const std::string& name) {

	if (!alreadyRegistered(name)) {
		// Add the command to the list (implicitly cast the pointer on Event&)
		_events[name] = IEventPtr(new WidgetToggle());

		// Return the pointer to the newly created event
		return _events[name];
	}

	return _emptyEvent;
}

IEventPtr EventManager::addRegistryToggle(const std::string& name, const std::string& registryKey)
{
	if (!alreadyRegistered(name)) {
		// Add the command to the list (implicitly cast the pointer on Event&)
		_events[name] = IEventPtr(new RegistryToggle(registryKey));

		// Return the pointer to the newly created event
		return _events[name];
	}

	return _emptyEvent;
}

IEventPtr EventManager::addToggle(const std::string& name, const ToggleCallback& onToggled)
{
	if (!alreadyRegistered(name)) {
		// Add the command to the list (implicitly cast the pointer on Event&)
		_events[name] = IEventPtr(new Toggle(onToggled));

		// Return the pointer to the newly created event
		return _events[name];
	}

	return _emptyEvent;
}

void EventManager::setToggled(const std::string& name, const bool toggled)
{
	// Check could be placed here by boost::shared_ptr's dynamic_pointer_cast
	if (!findEvent(name)->setToggled(toggled)) {
		rWarning() << "EventManager: Event " << name
			<< " is not a Toggle." << std::endl;
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
		for (Accelerator& accel : _accelerators)
        {
            if (accel.match(event))
            {
				// Connect the accelerator to the empty event (disable the accelerator)
                event->disconnectAccelerators();

				accel.setEvent(_emptyEvent);
				accel.setKey(0);
				accel.setModifiers(0);
			}
		}
	}
	else 
    {
		// Command NOT found
		rWarning() << "EventManager: Unable to disconnect command: " << command << std::endl;
	}
}

void EventManager::disableEvent(const std::string& eventName) {
	findEvent(eventName)->setEnabled(false);
}

void EventManager::enableEvent(const std::string& eventName) {
	findEvent(eventName)->setEnabled(true);
}

void EventManager::removeEvent(const std::string& eventName) {
	// Try to lookup the command
	EventMap::iterator i = _events.find(eventName);

	if (i != _events.end()) {
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
			pair.second->disconnectToolItem(const_cast<wxToolBarToolBase*>(toolbar->GetToolByPos(tool)));
		}
	});
}

// Loads the default shortcuts from the registry
void EventManager::loadAccelerators()
{
	if (_debugMode) {
        rConsole() << "EventManager: Loading accelerators..." << std::endl;
	}

	// Register all custom statements as events too to make them shortcut-bindable
	// before going ahead
	GlobalCommandSystem().foreachStatement([&] (const std::string& statementName)
	{
		addCommand(statementName, statementName, false);
	}, true); // custom statements only

	xml::NodeList shortcutSets = GlobalRegistry().findXPath("user/ui/input//shortcuts");

	if (_debugMode) {
        rConsole() << "Found " << shortcutSets.size() << " sets." << std::endl;
	}

	// If we have two sets of shortcuts, delete the default ones
	if (shortcutSets.size() > 1) {
		GlobalRegistry().deleteXPath("user/ui/input//shortcuts[@name='default']");
	}

	// Find all accelerators
	xml::NodeList shortcutList = GlobalRegistry().findXPath("user/ui/input/shortcuts//shortcut");

	if (!shortcutList.empty())
	{
		rMessage() << "EventManager: Shortcuts found in Registry: " << shortcutList.size() << std::endl;

		for (std::size_t i = 0; i < shortcutList.size(); i++)
		{
			const std::string key = shortcutList[i].getAttributeValue("key");
			const std::string cmd = shortcutList[i].getAttributeValue("command");

			if (_debugMode) 
			{
				rConsole() << "Looking up command: " << cmd << std::endl;
				rConsole() << "Key is: >> " << key << " << " << std::endl;
			}

			// Try to lookup the command
			IEventPtr event = findEvent(cmd);

			// Check for a non-empty key string
			if (!key.empty())
			{
				 // Check for valid command definitions were found
				if (!event->empty())
				{
					// Get the modifier string (e.g. "SHIFT+ALT")
					const std::string modifierStr = shortcutList[i].getAttributeValue("modifiers");

					if (!duplicateAccelerator(key, modifierStr, event))
					{
						// Create the accelerator object
						IAccelerator& accelerator = addAccelerator(key, modifierStr);

						// Connect the newly created accelerator to the command
                        event->connectAccelerator(accelerator);
                        static_cast<Accelerator&>(accelerator).setEvent(event);
					}
				}
				else
				{
					rWarning() << "EventManager: Cannot load shortcut definition (command invalid): " 
						<< cmd << std::endl;
				}
			}
		}
	}
	else 
	{
		// No accelerator definitions found!
		rWarning() << "EventManager: No shortcut definitions found..." << std::endl;
	}
}

void EventManager::foreachEvent(IEventVisitor& eventVisitor) {
	// Cycle through the event and pass them to the visitor class
	for (EventMap::iterator i = _events.begin(); i != _events.end(); i++) {
		const std::string eventName = i->first;
		IEventPtr event = i->second;

		eventVisitor.visit(eventName, event);
	}
}

// Tries to locate an accelerator, that is connected to the given command
IAccelerator& EventManager::findAccelerator(const IEventPtr& event)
{
	// Cycle through the accelerators and check for matches
    AcceleratorList::iterator end = _accelerators.end();
    for (AcceleratorList::iterator i = _accelerators.begin(); i != end; ++i)
    {
		if (i->match(event))
        {
			// Return the reference to the found accelerator
			return (*i);
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
	SaveEventVisitor visitor(rootKey, this);

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

	for (AcceleratorList::iterator i = accelList.begin(); i != accelList.end(); ++i)
    {
		// If one of the accelerators in the list matches the event, return true
		if (i->match(event))
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
	for (AcceleratorList::iterator i = _accelerators.begin(); i != _accelerators.end(); ++i)
    {
		if (i->match(keyVal, modifierFlags))
		{
			// Add the pointer to the found accelerators
			returnList.push_back((*i));
		}
	}

	return returnList;
}

AcceleratorList EventManager::findAccelerator(wxKeyEvent& ev)
{
	int keyval = ev.GetKeyCode(); // is always uppercase
	
	return findAccelerator(keyval, wxutil::Modifier::GetStateForKeyEvent(ev));
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
    returnValue += wxutil::Modifier::GetModifierStringForMenu(modifierFlags);
	returnValue += (returnValue != "") ? "-" : "";

	returnValue += getKeyString(ev);

	return returnValue;
}

}

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
    registry.registerModule(std::make_shared<ui::EventManager>());
    registry.registerModule(std::make_shared<ui::MouseToolManager>());

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
