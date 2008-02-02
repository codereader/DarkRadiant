#include "SpecifierEditCombo.h"
#include "specpanel/SpecifierPanelFactory.h"

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
	g_signal_connect(
		G_OBJECT(dropDown), "changed", G_CALLBACK(_onChange), this
	);
	
	// Main hbox
	_widget = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_widget), dropDown, TRUE, TRUE, 0);
}

// Get the main widget
GtkWidget* SpecifierEditCombo::getWidget() const {
	gtk_widget_show_all(_widget);
	return _widget;
}

/* GTK CALLBACKS */

void SpecifierEditCombo::_onChange(GtkWidget* w, SpecifierEditCombo* self)
{
	// Change the SpecifierPanel
	std::string selText = gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	self->_specPanel = SpecifierPanelFactory::create(selText);
	
	// If the panel is valid, get its widget and pack into the hbox
	if (self->_specPanel) {
		gtk_box_pack_end(
			GTK_BOX(self->_widget), self->_specPanel->getWidget(), TRUE, TRUE, 0
		);
	}
}

}

}
