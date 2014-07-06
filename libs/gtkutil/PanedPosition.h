#pragma once

#include <wx/event.h>

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
 * Use the loadFromNode() and saveToNode() methods to save the internal
 * size info into the given xml::Node
 *
 * This is used in various places (mainframe, entity inspector) to store 
 * the size/position of the paned views on shutdown.
 */
class PanedPosition :
	public wxEvtHandler
{
private:
	// The position of this object
	int _position;

	// The connected paned container
	wxSplitterWindow* _paned;

public:
	PanedPosition();

	~PanedPosition();

	// Connect the passed splitter window to this object
	void connect(wxSplitterWindow* paned);
	void disconnect(wxSplitterWindow* paned);

	const int getPosition() const;
	void setPosition(int position);

	void saveToPath(const std::string& path);
	void loadFromPath(const std::string& path);

	// Applies the internally stored size/position info to the GtkWindow
	// The algorithm was adapted from original GtkRadiant code (window.h)
	void applyPosition();

	// Reads the position from the GtkPaned and normalises it to the paned size
	void readPosition();

private:
	// The callback that gets invoked on position change
	void onPositionChange(wxSplitterEvent& ev);
};

} // namespace
