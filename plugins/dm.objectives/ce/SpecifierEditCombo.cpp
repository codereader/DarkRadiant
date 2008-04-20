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
SpecifierEditCombo::SpecifierEditCombo(const SpecifierTypeSet& set)
{
	// Create the dropdown containing specifier types
	_specifierCombo = objectives::util::TwoColumnTextCombo();

	GtkListStore* ls = GTK_LIST_STORE(
		gtk_combo_box_get_model(GTK_COMBO_BOX(_specifierCombo))
	);
	for (SpecifierTypeSet::const_iterator i = set.begin();
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
		G_OBJECT(_specifierCombo), "changed", G_CALLBACK(_onChange), this
	);
	
	// Main hbox
	_widget = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_widget), _specifierCombo, TRUE, TRUE, 0);
}

// Get the main widget
GtkWidget* SpecifierEditCombo::getWidget() const 
{
    gtk_widget_show_all(_widget);
	return _widget;
}

// Get the selected SpecifierType
const SpecifierType& SpecifierEditCombo::getSpecifier() const 
{
   return SpecifierType::getSpecifierType(getSpecName());
}

// Set the selected SpecifierType
void SpecifierEditCombo::setSpecifier(const SpecifierType& spec)
{
	// I copied and pasted this from the StimResponseEditor, the SelectionFinder
	// could be cleaned up a bit.
	gtkutil::TreeModel::SelectionFinder finder(spec.getName(), 1);
	gtk_tree_model_foreach(
		gtk_combo_box_get_model(GTK_COMBO_BOX(_specifierCombo)),
		gtkutil::TreeModel::SelectionFinder::forEach,
		&finder
	);
	
    // SpecifierType name should be found in list
    GtkTreePath* path = finder.getPath();
    assert(path);

    // Get an iter and set the selected item
    GtkTreeIter iter = finder.getIter();
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(_specifierCombo), &iter);
}

// Get the selected string value from the SpecifierPanel
std::string SpecifierEditCombo::getValue() const
{
    return _specPanel->getValue();
}

// Get the selected SpecifierType string
std::string SpecifierEditCombo::getSpecName() const
{
	// Get the current selection
	GtkTreeIter iter;
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(_specifierCombo), &iter);
	return gtkutil::TreeModel::getString(
			gtk_combo_box_get_model(GTK_COMBO_BOX(_specifierCombo)),
			&iter,
			1
	); 
}

/* GTK CALLBACKS */

void SpecifierEditCombo::_onChange(GtkWidget* w, SpecifierEditCombo* self)
{
	// Change the SpecifierPanel
	self->_specPanel = SpecifierPanelFactory::create(self->getSpecName());
	
	// If the panel is valid, get its widget and pack into the hbox
	if (self->_specPanel) {
		gtk_box_pack_end(
			GTK_BOX(self->_widget), self->_specPanel->getWidget(), TRUE, TRUE, 0
		);
	}
}

}

}
