#include "StatusBarManager.h"

#include "iradiant.h"
#include "itextstream.h"
#include "gtkutil/FramedWidget.h"

#include <gtkmm/box.h>
#include <gtkmm/table.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

namespace ui {

StatusBarManager::StatusBarManager() :
	_statusBar(Gtk::manage(new Gtk::Table(1, 1, false)))
{
	_statusBar->show_all();

	// greebo: Set the size request of the table to prevent it from "breaking" the window width
    // which can cause lockup problems on GTK+ 2.12
	_statusBar->set_size_request(100, -1);
}

Gtk::Widget* StatusBarManager::getStatusBar()
{
	return _statusBar;
}

void StatusBarManager::addElement(const std::string& name, Gtk::Widget* widget, int pos)
{
	// Get a free position
	int freePos = getFreePosition(pos);

	StatusBarElementPtr element(new StatusBarElement(widget));

	// Store this element
	_elements.insert(ElementMap::value_type(name, element));
	_positions.insert(PositionMap::value_type(freePos, element));

	rebuildStatusBar();
}

Gtk::Widget* StatusBarManager::getElement(const std::string& name)
{
	// Look up the key
	ElementMap::const_iterator found = _elements.find(name);

	// return NULL if not found
	return (found != _elements.end()) ? found->second->toplevel : NULL;
}

void StatusBarManager::addTextElement(const std::string& name, const std::string& icon, int pos)
{
	// Get a free position
	int freePos = getFreePosition(pos);

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	if (!icon.empty())
	{
		Gtk::Image* img = Gtk::manage(new Gtk::Image(
			GlobalUIManager().getLocalPixbuf(icon)
		));
		hbox->pack_start(*img, false, false, 0);
	}

	Gtk::Label* label = Gtk::manage(new Gtk::Label);
	hbox->pack_start(*label, false, false, 0);

	StatusBarElementPtr element(new StatusBarElement(Gtk::manage(new gtkutil::FramedWidget(*hbox)), label));

	// Store this element
	_elements.insert(ElementMap::value_type(name, element));
	_positions.insert(PositionMap::value_type(freePos, element));

	rebuildStatusBar();
}

void StatusBarManager::setText(const std::string& name, const std::string& text)
{
	// Look up the key
	ElementMap::const_iterator found = _elements.find(name);

	// return NULL if not found
	if (found != _elements.end() && found->second->label != NULL)
	{
		// Set the text
		found->second->text = text;

		// Request an idle callback
		requestIdleCallback();
	}
	else
	{
		rError() << "Could not find text status bar element with the name "
			<< name << std::endl;
	}
}

void StatusBarManager::onGtkIdle()
{
	// Fill in all buffered texts
	for (PositionMap::const_iterator i = _positions.begin(); i != _positions.end(); ++i)
	{
		// Shortcut
		const StatusBarElement& element = *(i->second);

		// Skip non-labels
		if (element.label == NULL) continue;

		element.label->set_markup(element.text);
	}
}

int StatusBarManager::getFreePosition(int desiredPosition)
{
	// Do we have an easy job?
	if (_positions.empty())
	{
		// nothing to calculate
		return desiredPosition;
	}

	PositionMap::const_iterator i = _positions.find(desiredPosition);

	if (i == _positions.end()) {
		return desiredPosition;
	}

	// Let's see if there's space between the desired position and the next larger one
	i = _positions.upper_bound(desiredPosition);

	if (i == _positions.end()) {
		// There is no position larger than the desired one, return this one
		return desiredPosition + 1;
	}
	// Found an existing position which is larger than the desired one
	else if (i->first == desiredPosition + 1) {
		// No space between these two items, put to back
		return _positions.rbegin()->first + 1;
	}
	else {
		return desiredPosition + 1;
	}
}

void StatusBarManager::rebuildStatusBar()
{
	// Prevent child widgets from destruction before clearing the container
	for (PositionMap::const_iterator i = _positions.begin(); i != _positions.end(); ++i)
	{
		// Grab a reference of the widgets (a new widget will be "floating")
		i->second->toplevel->reference();
	}

	_statusBar->foreach(sigc::mem_fun(*this, &StatusBarManager::_removeChildWidgets));

	if (_elements.empty()) return; // done here if empty

	// Resize the table to fit the widgets
	_statusBar->resize(1, static_cast<guint>(_elements.size()));

	int col = 0;

	for (PositionMap::const_iterator i = _positions.begin(); i != _positions.end(); ++i)
	{
		// Add the widget at the appropriate position
		_statusBar->attach(*i->second->toplevel, col, col+1, 0, 1);

		// Release the reference again, it's owned by the status bar (again)
		i->second->toplevel->unreference();

		col++;
	}

	_statusBar->show_all();
}

void StatusBarManager::_removeChildWidgets(Gtk::Widget& child)
{
	// Remove the visited child
	_statusBar->remove(child);
}

} // namespace ui
