#include "DialogBase.h"

#include <wx/display.h>
#include <wx/sizer.h>
#include <wx/frame.h>
#include "ui/imainframe.h"

#include "AutoSaveRequestBlocker.h"

namespace wxutil
{

namespace
{
    inline wxWindow* FindTopLevelWindow()
    {
        if (module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME))
        {
            return GlobalMainFrame().getWxTopLevelWindow();
        }

        return nullptr;
    }
}


DialogBase::DialogBase(const std::string& title) :
    DialogBase(title, nullptr, std::string()) // be a child of the DR top level window
{}

DialogBase::DialogBase(const std::string& title, const std::string& windowName) :
    DialogBase(title, nullptr, windowName) // be a child of the DR top level window
{}

DialogBase::DialogBase(const std::string& title, wxWindow* parent) :
    DialogBase(title, parent, std::string())
{}

DialogBase::DialogBase(const std::string& title, wxWindow* parent, const std::string& windowName)
: wxDialog(parent ? parent : FindTopLevelWindow(),
           wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
           wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER, !windowName.empty() ? windowName : wxString::FromAscii(wxDialogNameStr)),
    _windowState(windowName)
{
    // Allow subclasses to override close event
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) {
        if (_onDeleteEvent())
            e.Veto();
        else
            EndModal(wxID_CANCEL);
    });

    // Allow ESC to close all dialogs
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == WXK_ESCAPE)
            Close();
        else
            e.Skip();
    });

    _windowPosition.connect(this);
    _windowState.registerObject(&_windowPosition);
}

void DialogBase::FitToScreen(float xProp, float yProp)
{
    int curDisplayIdx = 0;

    if (GlobalMainFrame().getWxTopLevelWindow() != nullptr)
    {
        curDisplayIdx = wxDisplay::GetFromWindow(GlobalMainFrame().getWxTopLevelWindow());
    }

    wxDisplay curDisplay(curDisplayIdx);

    wxRect rect = curDisplay.GetGeometry();
    int newWidth = static_cast<int>(rect.GetWidth() * xProp);
    int newHeight = static_cast<int>(rect.GetHeight() * yProp);

    SetSize(newWidth, newHeight);
    CenterOnScreen();
}

int DialogBase::ShowModal()
{
    // While this dialog is active, block any auto save requests
    AutoSaveRequestBlocker blocker("Modal Dialog is active");

    _windowState.restore();

    auto returnCode = wxDialog::ShowModal();

    _windowState.save();

    return returnCode;
}

void DialogBase::RegisterPersistableObject(ui::IPersistableObject* object)
{
    _windowState.registerObject(object);
}

bool DialogBase::_onDeleteEvent()
{
    return false;
}

}
