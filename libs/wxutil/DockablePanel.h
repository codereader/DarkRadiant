#pragma once

#include <wx/panel.h>

namespace wxutil
{
/**
 * Base class of all panels that can either be floating or docked to the main area.
 * Examples: XY Views, Light Inspector, Media Browser.
 *
 * Each panel can be active or inactive. Inactive panels should not react to map events
 * not to unnecessarily waste processing time.
 * - Docked panels are always active.
 * - Tabbed panels are active if their tab is selected.
 * - Floating panels are active if they're not hidden.
 */
class DockablePanel :
    public wxPanel
{
private:
    bool _active;

public:
    DockablePanel(wxWindow* parent) :
        wxPanel(parent),
        _active(false)
    {}

    bool panelIsActive()
    {
        return _active;
    }

    // Ensure this panel is active.
    void activatePanel()
    {
        if (_active) return;

        _active = true;
        onPanelActivated();
    }

    // Set this panel to inactive.
    void deactivatePanel()
    {
        if (!_active) return;

        _active = false;
        onPanelDeactivated();
    }

protected:
    virtual void onPanelActivated()
    {}

    virtual void onPanelDeactivated()
    {}
};

} // namespace
