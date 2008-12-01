#include "CommandArgumentItem.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"

namespace ui {

CommandArgumentItem::CommandArgumentItem(
		const conversation::ArgumentInfo& argInfo, 
		GtkTooltips* tooltips) :
	_argInfo(argInfo),
	_tooltips(tooltips)
{
	// Pack the label into an eventbox
	_labelBox = gtk_event_box_new();
	GtkWidget* label = gtkutil::LeftAlignedLabel(_argInfo.title + ":");
	gtk_container_add(GTK_CONTAINER(_labelBox), label);
	
	gtk_tooltips_set_tip(_tooltips, _labelBox, argInfo.description.c_str(), "");
	
	// Pack the description widget into an eventbox
	_descBox = gtk_event_box_new();
	GtkWidget* descLabel = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(descLabel), "<b>?</b>");
	gtk_container_add(GTK_CONTAINER(_descBox), descLabel);
	
	gtk_tooltips_set_tip(_tooltips, _descBox, _argInfo.description.c_str(), "");
}

// Retrieve the label widget
GtkWidget* CommandArgumentItem::getLabelWidget() {
	return _labelBox;
}

GtkWidget* CommandArgumentItem::getHelpWidget() {
	return _descBox;
}

// StringArgument
StringArgument::StringArgument(
		const conversation::ArgumentInfo& argInfo, 
		GtkTooltips* tooltips) :
	CommandArgumentItem(argInfo, tooltips)
{
	_entry = gtk_entry_new();
	//gtk_entry_set_text(GTK_ENTRY(_entry), argInfo.value.c_str()); 
}

GtkWidget* StringArgument::getEditWidget() {
	return _entry;
}

std::string StringArgument::getValue() {
	return gtk_entry_get_text(GTK_ENTRY(_entry));
}

// Boolean argument
BooleanArgument::BooleanArgument(const conversation::ArgumentInfo& argInfo, GtkTooltips* tooltips) :
	 CommandArgumentItem(argInfo, tooltips)
{
	_checkButton = gtk_check_button_new_with_label(argInfo.title.c_str());
	//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_checkButton), !argInfo.value.empty());
}

GtkWidget* BooleanArgument::getEditWidget() {
	return _checkButton;
}

std::string BooleanArgument::getValue() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_checkButton)) ? "1" : "";
}

// Actor Argument
ActorArgument::ActorArgument(
		const conversation::ArgumentInfo& argInfo, 
		GtkTooltips* tooltips,
		GtkListStore* actorStore) :
	CommandArgumentItem(argInfo, tooltips),
	_actorStore(actorStore)
{
	// Cast the helper class onto a ListStore and create a new treeview
	_comboBox = gtk_combo_box_new_with_model(GTK_TREE_MODEL(_actorStore));
	
	// Add the cellrenderer for the name
	GtkCellRenderer* nameRenderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_comboBox), nameRenderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_comboBox), nameRenderer, "text", 1);
}

std::string ActorArgument::getValue() {
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(_comboBox), &iter)) {
		GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(_comboBox));
		std::string id = intToStr(gtkutil::TreeModel::getInt(model, &iter, 0));
		return id;
	}
	return "";
}

GtkWidget* ActorArgument::getEditWidget() {
	return _comboBox;
}

} // namespace ui
