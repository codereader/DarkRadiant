#include "EffectArgumentItem.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TreeModel.h"

EffectArgumentItem::EffectArgumentItem(
		ResponseEffect::Argument& arg, 
		GtkTooltips* tooltips) :
	_arg(arg),
	_tooltips(tooltips)
{
	// Pack the label into a eventbox
	_labelBox = gtk_event_box_new();
	GtkWidget* label = gtkutil::LeftAlignedLabel(_arg.title + ":");
	gtk_container_add(GTK_CONTAINER(_labelBox), label);
	
	gtk_tooltips_set_tip(_tooltips, _labelBox, arg.desc.c_str(), "");
	
	// Pack the description widget into a eventbox
	_descBox = gtk_event_box_new();
	GtkWidget* descLabel = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(descLabel), "<b>?</b>");
	gtk_container_add(GTK_CONTAINER(_descBox), descLabel);
	
	gtk_tooltips_set_tip(_tooltips, _descBox, arg.desc.c_str(), "");
}

// Retrieve the label widget
GtkWidget* EffectArgumentItem::getLabelWidget() {
	return _labelBox;
}

GtkWidget* EffectArgumentItem::getHelpWidget() {
	return _descBox;
}

void EffectArgumentItem::save() {
	// Save the value to the effect 
	_arg.value = getValue();
}

// StringArgument
StringArgument::StringArgument(
		ResponseEffect::Argument& arg, 
		GtkTooltips* tooltips) :
	EffectArgumentItem(arg, tooltips)
{
	_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(_entry), arg.value.c_str()); 
}

GtkWidget* StringArgument::getEditWidget() {
	return _entry;
}

std::string StringArgument::getValue() {
	return gtk_entry_get_text(GTK_ENTRY(_entry));
}

// Boolean argument
BooleanArgument::BooleanArgument(ResponseEffect::Argument& arg, GtkTooltips* tooltips) :
	 EffectArgumentItem(arg, tooltips)
{
	_checkButton = gtk_check_button_new_with_label(arg.title.c_str());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_checkButton), !arg.value.empty());
}

GtkWidget* BooleanArgument::getEditWidget() {
	return _checkButton;
}

std::string BooleanArgument::getValue() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_checkButton)) ? "1" : "";
}

// Entity Argument
EntityArgument::EntityArgument(
		ResponseEffect::Argument& arg, 
		GtkTooltips* tooltips,
		GtkListStore* entityStore) :
	EffectArgumentItem(arg, tooltips),
	_entityStore(entityStore)
{
	// Create a combo box entry with the given entity list
	_comboBox = gtk_combo_box_entry_new_with_model(
    				GTK_TREE_MODEL(_entityStore),
    				0); // number of the "text" column

	// Add completion functionality to the combobox entry
	GtkEntryCompletion* completion = gtk_entry_completion_new();
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(_entityStore));
	gtk_entry_completion_set_text_column(completion, 0);
	gtk_entry_set_completion(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(_comboBox))), 
							 completion);

	// Now select the entity passed in the argument
	// Find the entity using a TreeModel traversor (search the column #0)
	gtkutil::TreeModel::SelectionFinder finder(arg.value, 0);
	gtk_tree_model_foreach(
		GTK_TREE_MODEL(_entityStore), 
		gtkutil::TreeModel::SelectionFinder::forEach, 
		&finder
	);
	
	// Select the found treeiter, if the name was found in the liststore
	if (finder.getPath() != NULL) {
		GtkTreeIter iter = finder.getIter();
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(_comboBox), &iter);
	}
}

std::string EntityArgument::getValue() {
	return gtk_combo_box_get_active_text(GTK_COMBO_BOX(_comboBox));
}

GtkWidget* EntityArgument::getEditWidget() {
	return _comboBox;
}
