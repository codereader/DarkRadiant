#pragma once

#include "ieventmanager.h"
#include "Toggle.h"

#include <sigc++/connection.h>
#include <map>

namespace ui
{

/* greebo: A WidgetToggle can be connected to one or more toplevelwindows and shows/hides them
 * upon toggle (e.g. like the Camera Window).
 */
class WidgetToggle :
    public Toggle
{
    typedef std::set<wxTopLevelWindow*> Widgets;

    // The list of all the connected widgets
    Widgets _widgets;

public:
    // Constructor
    WidgetToggle();

    // Add a TopLevelWindow to the show/hide list
    virtual void connectTopLevelWindow(wxTopLevelWindow* widget);
    virtual void disconnectTopLevelWindow(wxTopLevelWindow* widget);

    virtual void updateWidgets();

private:
    void doNothing(bool) {}

    // Show/hide all the connected widgets
    void showWidgets();
    void hideWidgets();

    void readToggleStateFromWidgets();

    void visibilityChanged();
    void onVisibilityChange(wxShowEvent& ev);


}; // class WidgetToggle

}
