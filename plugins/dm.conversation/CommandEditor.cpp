#include "CommandEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftalignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Edit Command";
	}

CommandEditor::CommandEditor(GtkWindow* parent, conversation::ConversationCommand& command, conversation::Conversation conv) :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, parent),
	_conversation(conv),
	_command(command),
	_result(NUM_RESULTS),
	_actorStore(gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING)) // number + caption
{
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);

	// Fill the actor store
	for (conversation::Conversation::ActorMap::const_iterator i = _conversation.actors.begin();
		 i != _conversation.actors.end(); ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(_actorStore, &iter);
		gtk_list_store_set(_actorStore, &iter, 
						   0, i->first, 
						   1, (std::string("Actor ") + intToStr(i->first) + " (" + i->second + ")").c_str(),
						   -1);
	}

	// Create all widgets
	populateWindow();

	// Fill the values
	updateWidgets();

	// Show the editor and block
	show();
}

CommandEditor::Result CommandEditor::getResult() {
	return _result;
}

void CommandEditor::updateWidgets() {
	// Select the actor passed from the command
	// Find the entity using a TreeModel traversor (search the column #0)
	gtkutil::TreeModel::SelectionFinder finder(_command.actor, 0);

	gtk_tree_model_foreach(
		GTK_TREE_MODEL(_actorStore), 
		gtkutil::TreeModel::SelectionFinder::forEach, 
		&finder
	);
	
	// Select the found treeiter, if the name was found in the liststore
	if (finder.getPath() != NULL) {
		GtkTreeIter iter = finder.getIter();
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(_actorDropDown), &iter);
	}
}

void CommandEditor::save() {
	// TODO
}

void CommandEditor::populateWindow() {
	// Create the overall vbox
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Actor
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel("<b>Actor</b>"), FALSE, FALSE, 0);

	// Create the actor dropdown box
	_actorDropDown = gtk_combo_box_new_with_model(GTK_TREE_MODEL(_actorStore));

	// Add the cellrenderer for the name
	GtkCellRenderer* nameRenderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_actorDropDown), nameRenderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_actorDropDown), nameRenderer, "text", 1);
	
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(_actorDropDown, 18, 1), FALSE, FALSE, 0);

	// Command Type
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel("<b>Command</b>"), FALSE, FALSE, 0);
	//gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(createActorPanel(), 18, 1), FALSE, FALSE, 0);
	
	// Command Arguments
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel("<b>Command Arguments</b>"), FALSE, FALSE, 0);
	//gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(createCommandPanel(), 18, 1), TRUE, TRUE, 0);

	// Buttons
	gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);
}

GtkWidget* CommandEditor::createButtonPanel() {
	GtkWidget* buttonHBox = gtk_hbox_new(TRUE, 12);
	
	// Save button
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onSave), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, TRUE, TRUE, 0);
	
	// Cancel Button
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), cancelButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(buttonHBox);	
}

void CommandEditor::onSave(GtkWidget* button, CommandEditor* self) {
	// First, save to the command object
	self->_result = RESULT_OK;
	self->save();
	
	// Then close the window
	self->destroy();
}

void CommandEditor::onCancel(GtkWidget* button, CommandEditor* self) {
	// Just close the window without writing the values
	self->_result = RESULT_CANCEL;
	self->destroy();
}

} // namespace ui
