#pragma once

#include <wx/frame.h>
#include "gtkutil/WindowPosition.h"

namespace wxutil
{

class TransientWindow :
	public wxFrame
{
private:
	// Whether this window should be hidden rather than destroyed
	bool _hideOnDelete;

	// The window position tracker
	WindowPosition _windowPosition;

	// Registry key to load/save window position 
	std::string _windowStateKey;

protected:
	// Customisable virtuals implemented by subclasses
	virtual void _preShow();
	virtual void _postShow() { }

	virtual void _preHide();
	virtual void _postHide() { }

	virtual void _preDestroy() { }
	virtual void _postDestroy() { }

	// Return true to prevent the window from being deleted
	virtual bool _onDeleteEvent();

	virtual void _onSetFocus() { }

	// Set the default size and (if a key is given) load and apply the stored
	// window position from the registry
	void InitialiseWindowPosition(int defaultWidth, int defaultHeight, const std::string& windowStateKey);

	// Returns the registry key the window is saving the state to
	const std::string& GetWindowStateKey() const;

public:
	TransientWindow(const std::string& title, wxWindow* parent, bool hideOnDelete = false);

	virtual ~TransientWindow() {}

	// Override wxWindow::Show
	virtual bool Show(bool show = true);

	virtual void ToggleVisibility();

	virtual void SaveWindowState();

private:
	void _onDelete(wxCloseEvent& ev);
	void _onShowHide(wxShowEvent& ev);
	void _onFocus(wxFocusEvent& ev);
};

}
