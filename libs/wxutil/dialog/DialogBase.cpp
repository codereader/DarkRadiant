#include "DialogBase.h"

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

DialogBase::DialogBase(const std::string& title, wxWindow* parent)
: wxDialog(parent ? parent : FindTopLevelWindow(),
           wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
           wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER)
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
}

void DialogBase::FitToScreen(float xProp, float yProp)
{
    int curDisplayIdx = 0;

    if (GlobalMainFrame().getWxTopLevelWindow() != NULL)
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

}
