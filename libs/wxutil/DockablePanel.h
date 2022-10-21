#pragma once

#include <wx/panel.h>

namespace wxutil
{
/**
 * Base class of all panels that can either be floating or docked to the main area.
 * Examples: XY Views, Light Inspector, Media Browser.
 */
class DockablePanel :
    public wxPanel
{
public:
    DockablePanel(wxWindow* parent) :
        wxPanel(parent)
    {}
};

} // namespace
