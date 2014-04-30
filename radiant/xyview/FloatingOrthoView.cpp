#include "FloatingOrthoView.h"

#include "GlobalXYWnd.h"

namespace
{
	const std::string RKEY_XYVIEW_VIEW_ROOT = "user/ui/xyview/views";
}

FloatingOrthoView::FloatingOrthoView(int id, const std::string& title, wxWindow* parent) : 
	wxutil::TransientWindow(title, parent, false),
	XYWnd(id, this)
{
	std::string viewNodePath = RKEY_XYVIEW_VIEW_ROOT + "/view[@name='" + string::to_string(_id) + "']";

	InitialiseWindowPosition(-1, -1, viewNodePath);
}

void FloatingOrthoView::SaveWindowState()
{
	if (!GetWindowStateKey().empty())
	{
		// Prepare the registry, remove the previously existing key
		GlobalRegistry().deleteXPath(GetWindowStateKey());

		// Now create the new key
		GlobalRegistry().createKeyWithName(RKEY_XYVIEW_VIEW_ROOT, "view", string::to_string(_id));

		TransientWindow::SaveWindowState();

		// Persist the view type
		GlobalRegistry().setAttribute(GetWindowStateKey(), "type", getViewTypeStr(m_viewType));
	}
}

void FloatingOrthoView::setViewType(EViewType viewType)
{
	// Invoke the base class method first
	XYWnd::setViewType(viewType);

	// Get the title string and write it to the window
	SetTitle(getViewTypeTitle(viewType));
}

void FloatingOrthoView::_onSetFocus() 
{
	TransientWindow::_onSetFocus();

	// Let the global XYWndManager know about the focus change
	GlobalXYWnd().setActiveXY(_id);
}

// Post-destroy callback, initiate destruction of this XYWnd
void FloatingOrthoView::_postDestroy()
{
	// Tell the XYWndManager to release the shared_ptr of this instance.
	// Otherwise our destructor will never be called.
	GlobalXYWnd().destroyXYWnd(_id);
}

void FloatingOrthoView::onFocus(wxFocusEvent& ev)
{
	// Let the global XYWndManager know about the focus change
	GlobalXYWnd().setActiveXY(_id);
	ev.Skip();
}
