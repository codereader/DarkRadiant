#pragma once

#include "ui/iwindowstate.h"
#include <wx/event.h>
#include <wx/weakref.h>

class wxSplitterWindow;
class wxSplitterEvent;

namespace wxutil
{

/**
 * greebo: A PanedPosition object keeps track of the divider position of
 * a wxSplitterWindow object.
 *
 * Use the connect() method to connect a wxSplitterWindow to this object.
 *
 * Register this instance to a wxutil::WindowState helper to let the
 * divider position be saved to the registry on dialog close.
 *
 * This is used in various places (mainframe, entity inspector) to store 
 * the size/position of the paned views on shutdown.
 */
class PanedPosition :
	public wxEvtHandler,
    public ui::IPersistableObject
{
private:
    std::string _name;

	// The position of this object
	int _position;

	// The connected paned container
	wxWeakRef<wxSplitterWindow> _paned;

public:
	PanedPosition(const std::string& name = "splitter");

	~PanedPosition();

	// Connect the passed splitter window to this object
	void connect(wxSplitterWindow* paned);
	void disconnect();

	void setPosition(int position);

	void saveToPath(const std::string& path) override;
	void loadFromPath(const std::string& path) override;

private:
	// The callback that gets invoked on position change
	void onPositionChange(wxSplitterEvent& ev);
};

} // namespace
