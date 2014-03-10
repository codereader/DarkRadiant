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

#include <gtkmm/paned.h>
#include <sigc++/connection.h>
#include <string>

/* greebo: A PanedPosition object keeps track of the divider position.
 *
 * Use the connect() method to connect a Gtk::Paned to this object.
 *
 * Use the loadFromNode() and saveToNode() methods to save the internal
 * size info into the given xml::Node
 *
 * This is used by the Splitpane view (mainframe) to store the size/position of the
 * paned views on shutdown.
 */
namespace gtkutil
{

class PanedPosition
{
private:
	// The position of this object
	int _position;

	// The connected paned container
	Gtk::Paned* _paned;

	sigc::connection _connection;

public:
	PanedPosition();

	~PanedPosition();

	// Connect the passed GtkPaned to this object
	void connect(Gtk::Paned* paned);

	const int getPosition() const;
	void setPosition(int position);

	// Sets the position to the smallest possible value and applies it
	void applyMinPosition();

	// Sets the position to the largest possible value and applies it
	void applyMaxPosition();

	void saveToPath(const std::string& path);
	void loadFromPath(const std::string& path);

	// Applies the internally stored size/position info to the GtkWindow
	// The algorithm was adapted from original GtkRadiant code (window.h)
	void applyPosition();

	// Reads the position from the GtkPaned and normalises it to the paned size
	void readPosition();

private:
	// The callback that gets invoked on position change
	void onPositionChange();
};

} // namespace gtkutil

