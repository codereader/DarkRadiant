#include "MRUMenuItem.h"

#include "gtk/gtkbin.h"
#include "gtk/gtkmenuitem.h"
#include "gtk/gtklabel.h"
#include "gtk/gtkwidget.h"
#include "gtkutil/IConv.h"
#include "string/string.h"

#include "MRU.h"

namespace ui {

// Constructor
MRUMenuItem::MRUMenuItem(const std::string& label, ui::MRU& mru, unsigned int index) :
	_label(label),
	_mru(mru),
	_index(index),
	_widget(NULL)
{}

// Copy Constructor
MRUMenuItem::MRUMenuItem(const ui::MRUMenuItem& other) :
	_label(other._label),
	_mru(other._mru),
	_index(other._index),
	_widget(other._widget)
{}

void MRUMenuItem::setWidget(GtkWidget* widget) {
	_widget = widget;
}

MRUMenuItem::operator GtkWidget* () {
	return _widget;
}

void MRUMenuItem::activate(const cmd::ArgumentList& args) {
	// Only pass the call if the MenuItem is not the empty menu item (with index == 0)
	if (_label != "" && _index > 0) {
		// Pass the call back to the main MRU class to do the logistics
		_mru.loadMap(_label);
	}
}

void MRUMenuItem::show() {
	if (_widget != NULL) {
		gtk_widget_set_sensitive(_widget, _index > 0);
		gtk_widget_show_all(_widget);
	}
}

void MRUMenuItem::hide() {
	if (_widget != NULL) {
		gtk_widget_set_sensitive(_widget, false);
		gtk_widget_hide(_widget);
	}
}

void MRUMenuItem::setLabel(const std::string& label) {
	// Update the internal storage
	_label = label;
	
	// Add the number prefix to the widget label
	const std::string widgetLabel = intToStr(_index) + " - " + gtkutil::IConv::localeToUTF8(_label);
	
	GtkWidget* hbox = gtk_bin_get_child(GTK_BIN(_widget));
	
	// Get the list of child widgets
	GList* gtkChildren = gtk_container_get_children(GTK_CONTAINER(hbox));
	
	while (gtkChildren != NULL) {
		// Get the widget pointer from the current list item
		GtkWidget* candidate = reinterpret_cast<GtkWidget*>(gtkChildren->data);
		
		// Have we found the widget?
		if (GTK_IS_LABEL(candidate)) {
			gtk_label_set_text(GTK_LABEL(candidate), widgetLabel.c_str());
			break;
		}
		
		gtkChildren = gtkChildren->next;
	}
	
	// Show or hide the widget according to the actual string content
	if (_label != "") {
		show();
	}
	else {
		hide();
	}
}

std::string MRUMenuItem::getLabel() const {
	return _label;
}

int MRUMenuItem::getIndex() const {
	return _index;
}
	
} // namespace ui
