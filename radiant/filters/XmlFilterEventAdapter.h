#pragma once

#include "ieventmanager.h"
#include "itextstream.h"
#include "ifilter.h"

#include "XMLFilter.h"

namespace filters
{

/**
 * An object responsible for managing the commands and events 
 * bound to a single XMLFilter object.
 */
class XmlFilterEventAdapter
{
private:
    XMLFilter& _filter;

    IEventPtr _toggle;

public:
    typedef std::shared_ptr<XmlFilterEventAdapter> Ptr;

    XmlFilterEventAdapter(XMLFilter& filter) :
        _filter(filter)
    {
        // Add the corresponding toggle command to the eventmanager
        _toggle = createEventToggle();
    }

    ~XmlFilterEventAdapter()
    {
        // Remove all accelerators from that event before removal
        GlobalEventManager().disconnectAccelerator(_filter.getEventName());

        // Disable the event in the EventManager, to avoid crashes when calling the menu items
        GlobalEventManager().disableEvent(_filter.getEventName());
    }

    // Synchronisation routine to notify this class once the filter has been activated
    void setFilterState(bool isActive)
    {
        if (_toggle)
        {
            _toggle->setToggled(isActive);
        }
    }

    // Post-filter-rename event, to be invoked by the FilterSystem after a rename operation
    void onEventNameChanged(const std::string& oldEventName, const std::string& newEventName)
    {
        auto oldEvent = GlobalEventManager().findEvent(oldEventName);

        // Get the accelerator associated to the old event, if appropriate
        IAccelerator& oldAccel = GlobalEventManager().findAccelerator(oldEvent);

        // Add the toggle command to the eventmanager
        auto newToggle = createEventToggle();

        if (!newToggle->empty())
        {
            GlobalEventManager().connectAccelerator(oldAccel, newEventName);
        }
        else
        {
            rWarning() << "Can't register event after rename, the new event name is already registered!" << std::endl;
        }

        // Remove the old event from the EventManager
        GlobalEventManager().removeEvent(oldEventName);
    }

private:
    // The command target
    void toggle(bool newState)
    {
        GlobalFilterSystem().setFilterState(_filter.getName(), newState);
    }

    IEventPtr createEventToggle()
    {
        return GlobalEventManager().addToggle(
            _filter.getEventName(),
            std::bind(&XmlFilterEventAdapter::toggle, this, std::placeholders::_1)
        );
    }
};

}
