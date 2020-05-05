#include "XmlFilterEventAdapter.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "XMLFilter.h"
#include "BasicFilterSystem.h"
#include <fmt/format.h>

namespace filters
{

XmlFilterEventAdapter::XmlFilterEventAdapter(XMLFilter& filter) :
    _filter(filter)
{
    // Add the corresponding events/commands
    createToggleCommand();
    createSelectDeselectEvents();
}

XmlFilterEventAdapter::~XmlFilterEventAdapter()
{
    GlobalCommandSystem().removeCommand(_toggleCmdName);

#if 0
    if (!GlobalEventManager().findEvent(_filter.getEventName())->empty())
    {
        // Remove all accelerators from that event before removal
        GlobalEventManager().disconnectAccelerator(_filter.getEventName());

        // Disable the event in the EventManager, to avoid crashes when calling the menu items
        GlobalEventManager().disableEvent(_filter.getEventName());
    }
#endif

    removeSelectDeselectEvents();
}

void XmlFilterEventAdapter::setFilterState(bool isActive)
{
#if 0
    if (_toggle.second)
    {
        _toggle.second->setToggled(isActive);
    }
#endif
}

void XmlFilterEventAdapter::onEventNameChanged()
{
    GlobalCommandSystem().removeCommand(_toggleCmdName);
    createToggleCommand();

#if 0
    GlobalEventManager().renameEvent(_toggle.first, _filter.getEventName());
    _toggle.first = _filter.getEventName();
#endif

#if 0 // TODO CoreModule
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
#endif
}

void XmlFilterEventAdapter::toggle(bool newState)
{
    GlobalFilterSystem().setFilterState(_filter.getName(), newState);
}

void XmlFilterEventAdapter::createSelectDeselectEvents()
{
    // Select
    _selectByFilterCmd = fmt::format("{0}{1}", "SelectObjectBy", _filter.getEventName());

    GlobalCommandSystem().addStatement(_selectByFilterCmd, 
        fmt::format("{0} \"{1}\"", SELECT_OBJECTS_BY_FILTER_CMD, _filter.getName()), false
    );

    // Deselect
    _deselectByFilterCmd = fmt::format("{0}{1}", "DeselectObjectBy", _filter.getEventName());

    GlobalCommandSystem().addStatement(_deselectByFilterCmd,
        fmt::format("{0} \"{1}\"", DESELECT_OBJECTS_BY_FILTER_CMD, _filter.getName()), false
    );
}

void XmlFilterEventAdapter::removeSelectDeselectEvents()
{
    GlobalCommandSystem().removeCommand(_selectByFilterCmd);
    GlobalCommandSystem().removeCommand(_deselectByFilterCmd);
}

void XmlFilterEventAdapter::createToggleCommand()
{
    // Remember this name, the filter might change in the meantime
    _toggleCmdName = _filter.getEventName();
    GlobalCommandSystem().addStatement(_toggleCmdName,
        fmt::format("ToggleFilterState \"{0}\"", _filter.getName()), false);

#if 0
    _toggle = std::make_pair(_filter.getEventName(), GlobalEventManager().addToggle(
        _filter.getEventName(),
        std::bind(&XmlFilterEventAdapter::toggle, this, std::placeholders::_1)
    ));
#endif
}

}
