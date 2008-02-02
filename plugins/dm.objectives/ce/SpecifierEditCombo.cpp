#include "SpecifierEditCombo.h"

#include <gtk/gtk.h>

namespace objectives
{

namespace ce
{

// Constructor
SpecifierEditCombo::SpecifierEditCombo(const SpecifierSet& set)
{
	// Create the dropdown containing specifier types
	GtkWidget* dropDown = gtk_combo_box_new_text();
	for (SpecifierSet::const_iterator i = set.begin();
		 i != set.end();
		 ++i)
	{
		gtk_combo_box_append_text(
			GTK_COMBO_BOX(dropDown), i->getName().c_str()
		);
	}
	
	// Main hbox
	_widget = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_widget), dropDown, TRUE, TRUE, 0);
}

// Get the main widget
GtkWidget* SpecifierEditCombo::getWidget() const {
	gtk_widget_show_all(_widget);
	return _widget;
}

}

}
