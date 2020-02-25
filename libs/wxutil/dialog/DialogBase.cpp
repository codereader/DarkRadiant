#include "DialogBase.h"

namespace wxutil
{

void DialogBase::_onDelete(wxCloseEvent& ev)
{
    if (_onDeleteEvent())
    {
        ev.Veto();
    }
    else
    {
        EndModal(wxID_CANCEL);
    }
}

DialogBase::DialogBase(const std::string& title, wxWindow* parent)
: wxDialog(parent ? parent : GlobalMainFrame().getWxTopLevelWindow(),
           wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(DialogBase::_onDelete),
            nullptr, this);
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
