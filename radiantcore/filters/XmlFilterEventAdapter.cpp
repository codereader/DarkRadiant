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
    createToggleCommand();
    createSelectDeselectEvents();
}

XmlFilterEventAdapter::~XmlFilterEventAdapter()
{
    removeToggleCommand();
    removeSelectDeselectEvents();
}

void XmlFilterEventAdapter::onEventNameChanged()
{
    removeSelectDeselectEvents();
    removeToggleCommand();

    createToggleCommand();
    createSelectDeselectEvents();
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
}

void XmlFilterEventAdapter::removeToggleCommand()
{
    if (!_toggleCmdName.empty())
    {
        GlobalCommandSystem().removeCommand(_toggleCmdName);
        _toggleCmdName.clear();
    }
}

}
