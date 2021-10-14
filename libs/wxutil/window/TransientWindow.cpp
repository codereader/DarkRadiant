#include "TransientWindow.h"

#include "ui/imainframe.h"
#include "iregistry.h"
#include "wxutil/Bitmap.h"

namespace wxutil
{

TransientWindow::TransientWindow(const std::string& title, wxWindow* parent,
                                 bool hideOnDelete)
: wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
          wxSYSTEM_MENU | wxRESIZE_BORDER | wxCLOSE_BOX | wxCAPTION
#if defined(_WIN32)
          // Avoids minimisation problems on Windows, but results in child
          // window appearing unfocused on GTK, which is annoying.
          | wxFRAME_TOOL_WINDOW
#endif
          | wxCLIP_CHILDREN | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR),
  _hideOnDelete(hideOnDelete)
{
	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(TransientWindow::_onDelete), NULL, this);
	Connect(wxEVT_SHOW, wxShowEventHandler(TransientWindow::_onShowHide), NULL, this);

	CenterOnParent();

	// Set the window icon
	wxIcon appIcon;
	appIcon.CopyFromBitmap(wxutil::GetLocalBitmap("darkradiant_icon_64x64.png"));
	SetIcon(appIcon);
}

bool TransientWindow::Show(bool show)
{
	if (show)
	{
        // Restore the position
        _windowPosition.applyPosition();
		_preShow();
	}
	else
	{
        SaveWindowState();
		_preHide();
	}

	// Pass the call to base
	return wxFrame::Show(show);
}

void TransientWindow::_onShowHide(wxShowEvent& ev)
{
	ev.Skip();

	if (ev.IsShown())
	{
		_postShow();
	}
	else
	{
        // Bring the mainframe to foreground after closing this Window (#3965)
        // If we don't do this, some completely different application like Windows Explorer
        // might get the focus instead.
        if (GlobalMainFrame().getWxTopLevelWindow() != NULL)
        {
            GlobalMainFrame().getWxTopLevelWindow()->SetFocus();
        }
		_postHide();
	}
}

bool TransientWindow::_onDeleteEvent()
{
	if (_hideOnDelete)
    {
        Hide();
		return true; // veto event
    }

	_preDestroy();

	Destroy();

	_postDestroy();

	return false;
}

void TransientWindow::SaveWindowState()
{
	// Save the window position, to make sure
	_windowPosition.readPosition();

	// Tell the position tracker to save the information
	if (!_windowStateKey.empty())
	{
		_windowPosition.saveToPath(_windowStateKey);
	}
}

void TransientWindow::ToggleVisibility()
{
	if (!IsShownOnScreen())
	{
		Show();
	}
	else
	{
		Hide();
	}
}

void TransientWindow::InitialiseWindowPosition(int defaultWidth, int defaultHeight,
											   const std::string& windowStateKey)
{
	SetSize(defaultWidth, defaultHeight);
	Fit();

	_windowStateKey = windowStateKey;

	if (GlobalRegistry().keyExists(_windowStateKey))
	{
		// Connect the window position tracker
		_windowPosition.loadFromPath(_windowStateKey);
	}

	_windowPosition.connect(this);
}

const std::string& TransientWindow::GetWindowStateKey() const
{
	return _windowStateKey;
}

void TransientWindow::_onDelete(wxCloseEvent& ev)
{
	if (_onDeleteEvent())
	{
		ev.Veto();
	}
}

void TransientWindow::_onFocus(wxFocusEvent& ev)
{
	_onSetFocus();
	ev.Skip();
}

} // namespace
