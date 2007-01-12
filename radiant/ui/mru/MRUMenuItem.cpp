#include "MRUMenuItem.h"

#include "gtk/gtkbin.h"
#include "gtk/gtkmenuitem.h"
#include "gtk/gtklabel.h"
#include "gtk/gtkwidget.h"
#include "string/string.h"
#include <iostream>

#include "MRU.h"

namespace ui {

// Constructor
MRUMenuItem::MRUMenuItem(const std::string& label, ui::MRU& mru, unsigned int index) :
	_label(label),
	_mru(mru),
	_index(index),
	_menuItem(GTK_WIDGET(gtk_menu_item_new_with_label(_label.c_str())))
{
	show();
}

// Copy Constructor
MRUMenuItem::MRUMenuItem(const ui::MRUMenuItem& other) :
	_label(other._label),
	_mru(other._mru),
	_index(other._index),
	_menuItem(other._menuItem)
{
	show();
}

MRUMenuItem::operator GtkWidget* () {
	return _menuItem;
}

void MRUMenuItem::activate() {
	// Only pass the call if the MenuItem is not the empty menu item (with index == 0)
	if (_label != "" && _index > 0) {
		// Pass the call back to the main MRU class to do the logistics
		_mru.loadMap(_label);
	}
}

void MRUMenuItem::show() {
	gtk_widget_set_sensitive(_menuItem, _index > 0);
	gtk_widget_show_all(_menuItem);
}

void MRUMenuItem::hide() {
	gtk_widget_set_sensitive(_menuItem, false);
	gtk_widget_hide(_menuItem);
}

void MRUMenuItem::setLabel(const std::string& label) {
	// Update the internal storage
	_label = label;
	
	// Add the number prefix to the widget label
	const std::string widgetLabel = intToStr(_index) + " - " + _label;
	
	// Retrieve the widget's label and set the text
	GtkLabel* labelWidget = GTK_LABEL(gtk_bin_get_child(GTK_BIN(_menuItem)));
	gtk_label_set_text(labelWidget, widgetLabel.c_str());
	
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
	
} // namespace ui
