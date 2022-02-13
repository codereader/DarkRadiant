#pragma once

#include <wx/dialog.h>
#include <wx/frame.h>
#include <wx/display.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include "ui/imainframe.h"

namespace wxutil
{

/**
 * \brief Base class for many DarkRadiant dialogs.
 *
 * It comes with a panel/sizer combination pre-populated, use the _mainPanel
 * member to add more stuff.
 */
class DialogBase: public wxDialog
{
public:
    /// Construct and initialise
    DialogBase(const std::string& title, wxWindow* parent = NULL);

    /**
     * Adjust this window to fit the display DarkRadiant is currently (mainly)
     * active on.  Set the xProp and yProp factors to control how much space
     * this window should use.  The factors should be in the range (0.0..1.0],
     * where 1.0 takes the entire space.
     */
    void FitToScreen(float xProp, float yProp);

    virtual int ShowModal() override;

protected:
    // Overrideable: return true to prevent the window from being deleted
    virtual bool _onDeleteEvent()
    {
        return false;
    }
};

} // namespace
