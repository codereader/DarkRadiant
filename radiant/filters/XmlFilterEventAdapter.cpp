#include "XmlFilterEventAdapter.h"

#include "itextstream.h"
#include "XMLFilter.h"
#include "BasicFilterSystem.h"
#include <fmt/format.h>

namespace filters
{

XmlFilterEventAdapter::XmlFilterEventAdapter(XMLFilter& filter) :
    _filter(filter)
{
    // Add the corresponding events/commands
    createEventToggle();
    createSelectDeselectEvents();
}

XmlFilterEventAdapter::~XmlFilterEventAdapter()
{
    // Remove all accelerators from that event before removal
    GlobalEventManager().disconnectAccelerator(_filter.getEventName());

    // Disable the event in the EventManager, to avoid crashes when calling the menu items
    GlobalEventManager().disableEvent(_filter.getEventName());

    removeSelectDeselectEvents();
}

void XmlFilterEventAdapter::setFilterState(bool isActive)
{
    if (_toggle.second)
    {
        _toggle.second->setToggled(isActive);
    }
}

void XmlFilterEventAdapter::onEventNameChanged()
{
    GlobalEventManager().renameEvent(_toggle.first, _filter.getEventName());
    _toggle.first = _filter.getEventName();

    // Re-create the select/deselect events, keeping the accelerators intact
    // Can't use renameEvent here since the statement needs to be changed and there's no easy
    // way to assign a new statement to an existing event
    IAccelerator& oldSelectAccel = GlobalEventManager().findAccelerator(_selectByFilterCmd.second);
    IAccelerator& oldDeselectAccel = GlobalEventManager().findAccelerator(_deselectByFilterCmd.second);

    std::string oldSelectEvent = _selectByFilterCmd.first;
    std::string oldDeselectEvent = _deselectByFilterCmd.first;

    // Create a new set of events
    createSelectDeselectEvents();

    // Assign the old shortcuts
    GlobalEventManager().connectAccelerator(oldSelectAccel, _selectByFilterCmd.first);
    GlobalEventManager().connectAccelerator(oldDeselectAccel, _deselectByFilterCmd.first);

    // Remove the old events
    GlobalEventManager().removeEvent(oldSelectEvent);
    GlobalEventManager().removeEvent(oldDeselectEvent);
}

void XmlFilterEventAdapter::toggle(bool newState)
{
    GlobalFilterSystem().setFilterState(_filter.getName(), newState);
}

void XmlFilterEventAdapter::createSelectDeselectEvents()
{
    // Select
    std::string eventName = fmt::format("{0}{1}", "SelectObjectBy", _filter.getEventName());

    _selectByFilterCmd = std::make_pair(eventName, GlobalEventManager().addCommand(
        eventName,
        fmt::format("{0} \"{1}\"", SELECT_OBJECTS_BY_FILTER_CMD, _filter.getName())
    ));

    // Deselect
    eventName = fmt::format("{0}{1}", "DeselectObjectBy", _filter.getEventName());

    _deselectByFilterCmd = std::make_pair(eventName, GlobalEventManager().addCommand(
        eventName,
        fmt::format("{0} \"{1}\"", DESELECT_OBJECTS_BY_FILTER_CMD, _filter.getName())
    ));
}

void XmlFilterEventAdapter::removeSelectDeselectEvents()
{
    GlobalEventManager().removeEvent(_selectByFilterCmd.first);
    GlobalEventManager().removeEvent(_deselectByFilterCmd.first);
}

void XmlFilterEventAdapter::createEventToggle()
{
    _toggle = std::make_pair(_filter.getEventName(), GlobalEventManager().addToggle(
        _filter.getEventName(),
        std::bind(&XmlFilterEventAdapter::toggle, this, std::placeholders::_1)
    ));
}

}
