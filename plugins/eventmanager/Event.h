#pragma once

#include "Accelerator.h"
#include <typeinfo>
#include <algorithm>
#include <iostream>
#include <wx/menuitem.h>
#include <wx/toolbar.h>
#include <regex>

namespace ui
{

/* The base class for an Event.
 *
 * Provides methods to enable/disable the event and to connect wxWidgets
 */
class Event :
    public IEvent
{
protected:
    // If false, the command is ignored and not executed.
    bool _enabled;

public:

    // Constructor
    Event() :
        _enabled(true)
    {}

    virtual ~Event() {}

    // Enables/disables this command according to the passed bool <enabled>
    virtual void setEnabled(const bool enabled) {
        _enabled = enabled;
    }

    virtual bool isToggle() const {
        return false;
    }

    virtual bool setToggled(const bool toggled) {
        return false;
    }

    virtual bool empty() const {
        // This is the base class for an event, it's empty by default
        return true;
    }

    virtual void keyUp() {}
    virtual void keyDown() {}

    virtual void updateWidgets() {}

    // Empty standard implementation
    virtual void connectTopLevelWindow(wxTopLevelWindow* widget) {}
    virtual void disconnectTopLevelWindow(wxTopLevelWindow* widget) {}

    virtual void connectMenuItem(wxMenuItem* item) {}
    virtual void disconnectMenuItem(wxMenuItem* item) {}

    virtual void connectToolItem(wxToolBarToolBase* item) {}
    virtual void disconnectToolItem(wxToolBarToolBase* item) {}

    virtual void connectButton(wxButton* button) {}
    virtual void disconnectButton(wxButton* button) {}

    virtual void connectToggleButton(wxToggleButton* button) {}
    virtual void disconnectToggleButton(wxToggleButton* button) {}

    virtual void connectAccelerator(IAccelerator& accel) {}
    virtual void disconnectAccelerators() {}

protected:
    static void setMenuItemAccelerator(wxMenuItem* item, Accelerator& accel)
    {
        // Cut off any existing accelerators
        wxString caption = item->GetItemLabel().BeforeFirst('\t');

        wxString accelText = accel.getAcceleratorString(true);

        // greebo: Accelerators seem to globally catch the key events, add a space to fool wxWidgets
        item->SetItemLabel(caption + "\t " + accelText);
    }

    static void clearMenuItemAccelerator(wxMenuItem* item)
    {
        item->SetItemLabel(item->GetItemLabel().BeforeFirst('\t'));
    }

    static void setToolItemAccelerator(wxToolBarToolBase* tool, Accelerator& accel)
    {
        wxString accelText = accel.getAcceleratorString(true);
        std::replace(accelText.begin(), accelText.end(), '~', '-');

        tool->SetShortHelp(getCleanToolItemHelpText(tool) + " (" + accelText + ")");
    }

    static void clearToolItemAccelerator(wxToolBarToolBase* tool)
    {
        // Remove the accelerator from this tool
        tool->SetShortHelp(getCleanToolItemHelpText(tool));
    }

    static std::string getCleanToolItemHelpText(wxToolBarToolBase* tool)
    {
        std::string prevHelp = tool->GetShortHelp().ToStdString();

        // Use a regex to cut off the trailing " (Ctrl-X)"
        std::regex expr("\\s\\(.+\\)$");
        return std::regex_replace(prevHelp, expr, "");
    }
};

}
