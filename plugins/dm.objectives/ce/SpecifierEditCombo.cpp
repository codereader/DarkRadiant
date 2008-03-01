#include "SpecifierEditCombo.h"
#include "specpanel/SpecifierPanelFactory.h"
#include "../util/TwoColumnTextCombo.h"

#include "gtkutil/TreeModel.h"

#include <gtk/gtk.h>

namespace objectives
{

namespace ce
{

// Constructor
SpecifierEditCombo::SpecifierEditCombo(const SpecifierSet& set)
{
	// Create the dropdown containing specifier types
	GtkWidget* dropDown = objectives::util::TwoColumnTextCombo();

	GtkListStore* ls = GTK_LIST_STORE(
		gtk_combo_box_get_model(GTK_COMBO_BOX(dropDown))
	);
	for (SpecifierSet::const_iterator i = set.begin();
		 i != set.end();
		 ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(ls, &iter);
		gtk_list_store_set(
			ls, &iter, 
			0, i->getDisplayName().c_str(), 
			1, i->getName().c_str(),
			-1
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
	// Get the current selection
	GtkTreeIter iter;
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(w), &iter);
	std::string selText = gtkutil::TreeModel::getString(
			gtk_combo_box_get_model(GTK_COMBO_BOX(w)),
			&iter,
			1
	); 

	// Change the SpecifierPanel
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
