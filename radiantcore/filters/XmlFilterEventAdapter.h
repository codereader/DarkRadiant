#pragma once

#include <utility>
#include <string>

#include "ifilter.h"

namespace filters
{

class XMLFilter;

/**
 * An object responsible for managing the commands 
 * bound to a single XMLFilter object.
 */
class XmlFilterEventAdapter
{
private:
    XMLFilter& _filter;

    std::string _toggleCmdName;
    std::string _selectByFilterCmd;
    std::string _deselectByFilterCmd;

public:
    typedef std::shared_ptr<XmlFilterEventAdapter> Ptr;

    XmlFilterEventAdapter(XMLFilter& filter);

    ~XmlFilterEventAdapter();

    // Post-filter-rename event, to be invoked by the FilterSystem after a rename operation
    void onEventNameChanged();

private:
    void createSelectDeselectEvents();
    void removeSelectDeselectEvents();
    void createToggleCommand();
    void removeToggleCommand();
};

}
