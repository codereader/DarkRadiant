#include "TransientWindow.h"

#include "iuimanager.h"
#include "iregistry.h"
#include <wx/artprov.h>

namespace wxutil
{

TransientWindow::TransientWindow(const std::string& title, wxWindow* parent, bool hideOnDelete) :
	wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, 
		wxSYSTEM_MENU | wxRESIZE_BORDER | wxMINIMIZE_BOX | 
		wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN | wxFRAME_FLOAT_ON_PARENT | 
		wxFRAME_NO_TASKBAR),
	_hideOnDelete(hideOnDelete)
{
	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(TransientWindow::_onDelete), NULL, this);
	Connect(wxEVT_SHOW, wxShowEventHandler(TransientWindow::_onShowHide), NULL, this);

	CenterOnParent();

	// Set the window icon
	wxIcon appIcon;
	appIcon.CopyFromBitmap(wxArtProvider::GetBitmap(
		GlobalUIManager().ArtIdPrefix() + "darkradiant_icon_64x64.png"));
	SetIcon(appIcon);
}

bool TransientWindow::Show(bool show)
{
	if (show)
	{
		_preShow();
	}
	else
	{
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

void TransientWindow::_preShow()
{
	// Restore the position
	_windowPosition.applyPosition();
}

void TransientWindow::_preHide()
{
	SaveWindowState();
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
	_windowPosition.applyPosition();
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
