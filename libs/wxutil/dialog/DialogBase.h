#pragma once

#include <string>
#include <wx/dialog.h>
#include <wx/frame.h>
#include "../WindowPosition.h"
#include "../WindowState.h"

namespace wxutil
{

/**
 * \brief Base class for many DarkRadiant dialogs.
 *
 * This base implementation will listen to ESC keys to close the dialog.
 *
 * If a dialog name has been passed to the constructor, the window will 
 * save its state to the registry on close (and restore it in ShowModal()).
 */
class DialogBase : public wxDialog
{
private:
    WindowState _windowState;
    WindowPosition _windowPosition;

public:
    /// Construct and initialise a dialog that is a child of DarkRadiant's main window (if present)
    DialogBase(const std::string& title);

    // Construct a dialog that is child of the given parent window (if the window is nullptr, the
    // dialog will try to be a child to DarkRadiant's main window)
    DialogBase(const std::string& title, wxWindow* parent);

    // Construct and initialise a named dialog that is a child of DarkRadiant's main window (if present)
    // The name should not have any spaces or special characters in it, keep it alphanumeric
    DialogBase(const std::string& title, const std::string& windowName);

    // Construct a named dialog that is child of the given parent (if parent is nullptr, the
    // dialog will try to be a child to DarkRadiant's main window)
    // The name should not have any spaces or special characters in it, keep it alphanumeric
    DialogBase(const std::string& title, wxWindow* parent, const std::string& windowName);

    /**
     * Adjust this window to fit the display DarkRadiant is currently (mainly)
     * active on.  Set the xProp and yProp factors to control how much space
     * this window should use.  The factors should be in the range (0.0..1.0],
     * where 1.0 takes the entire space.
     */
    void FitToScreen(float xProp, float yProp);

    int ShowModal() override;

protected:
    // Adds an element that should persist its state between dialog sessions
    void RegisterPersistableObject(ui::IPersistableObject* object);

    // Overrideable: return true to prevent the window from being deleted
    virtual bool _onDeleteEvent();
};

} // namespace
