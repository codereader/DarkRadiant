#include "FloatingOrthoView.h"

#include "GlobalXYWnd.h"

namespace ui
{

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
		GlobalRegistry().setAttribute(GetWindowStateKey(), "type", getViewTypeStr(_viewType));
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

bool FloatingOrthoView::_onDeleteEvent()
{
    // Don't call base class, just issue the call to GlobalXYWnd
    // which will call the destructor.
    GlobalXYWnd().destroyXYWnd(_id);

    return true; // veto this event
}

void FloatingOrthoView::onFocus(wxFocusEvent& ev)
{
	// Let the global XYWndManager know about the focus change
	GlobalXYWnd().setActiveXY(_id);
	ev.Skip();
}

} // namespace