#include "ConversationEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftalignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"

namespace ui {

namespace {
	const std::string WINDOW_TITLE = "Edit Conversation";
	const int MIN_HEIGHT_ACTORS_TREEVIEW = 160;
	const int MIN_HEIGHT_COMMAND_TREEVIEW = 200;

	enum {
		WIDGET_CONV_NAME_ENTRY,
		WIDGET_CONV_ACTOR_WITHIN_TALKDIST,
		WIDGET_CONV_ACTORS_ALWAYS_FACE,
		WIDGET_ADD_ACTOR_BUTTON,
		WIDGET_DELETE_ACTOR_BUTTON,
	};
}

ConversationEditor::ConversationEditor(GtkWindow* parent, conversation::Conversation& conversation) :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, parent),
	_actorStore(gtk_list_store_new(2, 
								   G_TYPE_INT,		// actor number
								   G_TYPE_STRING)),	// display name
   _commandStore(gtk_list_store_new(3, 
								   G_TYPE_INT,		// cmd number
								   G_TYPE_STRING,	// actor name
								   G_TYPE_STRING)),	// sentence
   _conversation(conversation)
{
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);

	// Create the widgets
	populateWindow();

	// Load the conversation values into the widgets
	updateWidgets();

	// Show and block
	show();
}

void ConversationEditor::populateWindow() {
	// Create the overall vbox
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Create the conversation properties pane
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel("<b>Conversation Properties</b>"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(createPropertyPane(), 18, 1), FALSE, FALSE, 0);

	// Actors
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel("<b>Actors</b>"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(createActorPanel(), 18, 1), FALSE, FALSE, 0);
	
	// Commands
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel("<b>Commands</b>"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(createCommandPanel(), 18, 1), TRUE, TRUE, 0);

	// Buttons
	gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);
}

GtkWidget* ConversationEditor::createPropertyPane() {
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Table for entry boxes
	GtkWidget* table = gtk_table_new(4, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);

	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	int row = 0;
	
	// Conversation name
	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel("Name"),
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	_widgets[WIDGET_CONV_NAME_ENTRY] = gtk_entry_new();
	gtk_table_attach_defaults(GTK_TABLE(table), 
							  _widgets[WIDGET_CONV_NAME_ENTRY], 
							  1, 2, row, row+1);
	
	row++;

	// Actors within talk distance
	_widgets[WIDGET_CONV_ACTOR_WITHIN_TALKDIST] = gtk_check_button_new();
	gtk_table_attach(GTK_TABLE(table), 
			gtkutil::RightAlignment(_widgets[WIDGET_CONV_ACTOR_WITHIN_TALKDIST]),
			0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), 
							  gtkutil::LeftAlignedLabel("Actors must be within talk distance"), 
							  1, 2, row, row+1);
	
	row++;

	// Actors always face each other while talking
	_widgets[WIDGET_CONV_ACTORS_ALWAYS_FACE] = gtk_check_button_new();
	gtk_table_attach(GTK_TABLE(table), 
			gtkutil::RightAlignment(_widgets[WIDGET_CONV_ACTORS_ALWAYS_FACE]),
			0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), 
							  gtkutil::LeftAlignedLabel("Actors always face each other while talking"), 
							  1, 2, row, row+1);
	
	row++;

	return vbox;
}

GtkWidget* ConversationEditor::createActorPanel() {
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);
	
	// Tree view
	GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_actorStore));
	gtk_widget_set_size_request(tv, -1, MIN_HEIGHT_ACTORS_TREEVIEW);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), TRUE);

	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(onActorSelectionChanged), this);
	
	// Key and value text columns
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn("#", 0, false));

	// Construct a new editable text column
	gtkutil::TextColumn actorColumn("Actor (click to edit)", 1, false);
	
	GtkCellRendererText* rend = actorColumn.getCellRenderer();
	g_object_set(G_OBJECT(rend), "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(onActorEdited), this);

	// Cast the column object to a GtkTreeViewColumn* and append it
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), actorColumn);
		
	// Action buttons
	_widgets[WIDGET_ADD_ACTOR_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_ADD);
	_widgets[WIDGET_DELETE_ACTOR_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_ADD_ACTOR_BUTTON]), "clicked", G_CALLBACK(onAddActor), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_DELETE_ACTOR_BUTTON]), "clicked", G_CALLBACK(onDeleteActor), this);

	GtkWidget* actionVBox = gtk_vbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_ADD_ACTOR_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_DELETE_ACTOR_BUTTON], FALSE, FALSE, 0);

	// Actors treeview goes left, actionbuttons go right
	gtk_box_pack_start(GTK_BOX(hbox), gtkutil::ScrolledFrame(tv), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), actionVBox, FALSE, FALSE, 0);
		
	return hbox;
}

GtkWidget* ConversationEditor::createCommandPanel() {
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);
	
	// Tree view
	GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_commandStore));
	gtk_widget_set_size_request(tv, -1, MIN_HEIGHT_COMMAND_TREEVIEW);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), TRUE);

	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(onCommandSelectionChanged), this);
	
	// Key and value text columns
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn("#", 0, false));
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn("Actor", 1));
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn("Command", 2));
	
	// Action buttons
	GtkWidget* actionVBox = gtk_vbox_new(FALSE, 6);

	// TODO

	// Command treeview goes left, action buttons go right
	gtk_box_pack_start(GTK_BOX(hbox), gtkutil::ScrolledFrame(tv), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), actionVBox, FALSE, FALSE, 0);
		
	return hbox;
}

GtkWidget* ConversationEditor::createButtonPanel() {
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

void ConversationEditor::updateWidgets() {
	// Clear the liststores first
	gtk_list_store_clear(_actorStore);
	gtk_list_store_clear(_commandStore);

	// Name
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_CONV_NAME_ENTRY]), _conversation.name.c_str());

	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_CONV_ACTOR_WITHIN_TALKDIST]),
		_conversation.actorsMustBeWithinTalkdistance ? TRUE : FALSE
	);

	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_widgets[WIDGET_CONV_ACTORS_ALWAYS_FACE]),
		_conversation.actorsAlwaysFaceEachOther ? TRUE : FALSE
	);

	// Actors
	for (conversation::Conversation::ActorMap::const_iterator i = _conversation.actors.begin();
		 i != _conversation.actors.end(); ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(_actorStore, &iter);
		gtk_list_store_set(_actorStore, &iter, 
						   0, i->first, 
						   1, i->second.c_str(),
						   -1);
	}

	// Commands
	for (conversation::Conversation::CommandMap::const_iterator i = _conversation.commands.begin();
		 i != _conversation.commands.end(); ++i)
	{
		const conversation::ConversationCommand& cmd = *(i->second);

		GtkTreeIter iter;
		gtk_list_store_append(_commandStore, &iter);
		gtk_list_store_set(_commandStore, &iter, 
						   0, i->first, 
						   1, (std::string("Actor ") + intToStr(cmd.actor)).c_str(),
						   2, cmd.getSentence().c_str(),
						   -1);
	}
}

void ConversationEditor::save() {
	// Name
	_conversation.name = gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_CONV_NAME_ENTRY]));

	_conversation.actorsMustBeWithinTalkdistance = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_CONV_ACTOR_WITHIN_TALKDIST])) ? true : false;

	_conversation.actorsAlwaysFaceEachOther = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_CONV_ACTORS_ALWAYS_FACE])) ? true : false;
}

void ConversationEditor::onSave(GtkWidget* button, ConversationEditor* self) {
	// First, save to the conversation object
	self->save();

	// Then close the window
	self->destroy();
}

void ConversationEditor::onCancel(GtkWidget* button, ConversationEditor* self) {
	// Just close the window without writing the values
	self->destroy();
}

void ConversationEditor::onActorSelectionChanged(GtkTreeSelection* sel, ConversationEditor* self) {
	// Get the selection
	bool hasSelection = gtk_tree_selection_get_selected(sel, NULL, &(self->_currentActor)) ? true : false;

	// Enable the delete buttons if we have a selection
	gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_ACTOR_BUTTON], hasSelection ? TRUE: FALSE);
}

void ConversationEditor::onCommandSelectionChanged(GtkTreeSelection* sel, ConversationEditor* self) {
	// TODO: Set button sensitivity
}

void ConversationEditor::onAddActor(GtkWidget* w, ConversationEditor* self) {
	// Get the lowest available actor ID
	int idx = 1;

	for (idx = 1; idx < INT_MAX; idx++) {
		if (self->_conversation.actors.find(idx) == self->_conversation.actors.end()) {
			break;
		}
	}

	// Add the new actor to the map
	self->_conversation.actors[idx] = "New Actor";

	// Update the widgets
	self->updateWidgets();
}

void ConversationEditor::onDeleteActor(GtkWidget* w, ConversationEditor* self) {
	// Get the index of the currently selected actor
	int index = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(self->_actorStore), &(self->_currentActor), 0);

	// Add the new actor to the map
	conversation::Conversation::ActorMap::iterator i = self->_conversation.actors.find(index);

	if (i != self->_conversation.actors.end()) {
		// Remove the specified actor
		self->_conversation.actors.erase(index);
	}

	// Adjust the numbers of all other actors with higher numbers
	while (self->_conversation.actors.find(index + 1) != self->_conversation.actors.end()) {
		// Move the actor with the higher index "down" by one number...
		self->_conversation.actors[index] = self->_conversation.actors[index + 1];
		// ...and remove it from the old location
		self->_conversation.actors.erase(index + 1);

		index++;
	}
	
	// Update the widgets
	self->updateWidgets();
}

void ConversationEditor::onActorEdited(GtkCellRendererText* renderer, 
	gchar* path, gchar* new_text, ConversationEditor* self) 
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(self->_actorStore), &iter, path)) {
		// The iter points to the edited cell now, get the actor number
		int actorNum = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(self->_actorStore), &iter, 0);
		
		// Update the conversation
		self->_conversation.actors[actorNum] = new_text;

		// Update all widgets
		self->updateWidgets();
	}
}

} // namespace ui
