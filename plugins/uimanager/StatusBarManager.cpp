#include "StatusBarManager.h"

#include "iradiant.h"
#include "itextstream.h"
#include "gtkutil/FramedWidget.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtktable.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>

namespace ui {

StatusBarManager::StatusBarManager() :
	_statusBar(gtk_table_new(1, 1, FALSE))
{
	gtk_widget_show_all(_statusBar);

	// greebo: Set the size request of the table to prevent it from "breaking" the window width
    // which can cause lockup problems on GTK+ 2.12
	gtk_widget_set_size_request(_statusBar, 100, -1);
}

GtkWidget* StatusBarManager::getStatusBar() {
	return _statusBar;
}

void StatusBarManager::addElement(const std::string& name, GtkWidget* widget, int pos) {
	// Get a free position
	int freePos = getFreePosition(pos);

	StatusBarElementPtr element(new StatusBarElement(widget));

	// Store this element
	_elements.insert(ElementMap::value_type(name, element));
	_positions.insert(PositionMap::value_type(freePos, element));

	rebuildStatusBar();
}

GtkWidget* StatusBarManager::getElement(const std::string& name) {
	// Look up the key
	ElementMap::const_iterator found = _elements.find(name);
	
	// return NULL if not found
	return (found != _elements.end()) ? found->second->toplevel : NULL;
}

void StatusBarManager::addTextElement(const std::string& name, const std::string& icon, int pos) {
	// Get a free position
	int freePos = getFreePosition(pos);

	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	if (!icon.empty()) {
		GtkWidget* img = gtk_image_new_from_pixbuf(
			GlobalUIManager().getLocalPixbuf(icon)
		);
		gtk_box_pack_start(GTK_BOX(hbox), img, FALSE, FALSE, 0);
	}

	GtkWidget* label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	StatusBarElementPtr element(new StatusBarElement(gtkutil::FramedWidget(hbox), label));

	// Store this element
	_elements.insert(ElementMap::value_type(name, element));
	_positions.insert(PositionMap::value_type(freePos, element));

	rebuildStatusBar();
}

void StatusBarManager::setText(const std::string& name, const std::string& text) {
	// Look up the key
	ElementMap::const_iterator found = _elements.find(name);
	
	// return NULL if not found
	if (found != _elements.end() && found->second->label != NULL) {
		// Set the text
		found->second->text = text;

		// Request an idle callback
		requestIdleCallback();
	}
	else {
		globalErrorStream() << "Could not find text status bar element with the name " 
			<< name << std::endl;
	}
}

void StatusBarManager::onGtkIdle() {
	// Fill in all buffered texts
	for (PositionMap::const_iterator i = _positions.begin(); i != _positions.end(); ++i) {
		// Shortcut
		const StatusBarElement& element = *(i->second);

		// Skip non-labels
		if (element.label == NULL) continue;
		
		gtk_label_set_markup(GTK_LABEL(element.label), element.text.c_str());
	}
}

int StatusBarManager::getFreePosition(int desiredPosition) {
	// Do we have an easy job?
	if (_positions.empty()) {
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

void StatusBarManager::rebuildStatusBar() {
	// Prevent child widgets from destruction before clearing the container
	for (PositionMap::const_iterator i = _positions.begin(); i != _positions.end(); ++i) {
		// Grab a reference of the widgets (a new widget will be "floating")
		g_object_ref_sink(i->second->toplevel);
	}

	gtk_container_foreach(GTK_CONTAINER(_statusBar), _removeChildWidgets, _statusBar);

	if (_elements.empty()) return; // done here if empty

	// Resize the table to fit the widgets
	gtk_table_resize(GTK_TABLE(_statusBar), 1, static_cast<int>(_elements.size()));

	int col = 0;
	for (PositionMap::const_iterator i = _positions.begin(); i != _positions.end(); ++i) {
		// Add the widget at the appropriate position
		gtk_table_attach_defaults(GTK_TABLE(_statusBar), i->second->toplevel, col, col+1, 0, 1);

		// Release the reference again, it's owned by the status bar (again)
		g_object_unref(i->second->toplevel);

		col++;
	}

	gtk_widget_show_all(_statusBar);
}

void StatusBarManager::_removeChildWidgets(GtkWidget* child, gpointer statusBar) {
	// Remove the visited child
	gtk_container_remove(GTK_CONTAINER(statusBar), child);
}

} // namespace ui
