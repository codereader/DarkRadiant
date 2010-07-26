#include "ConversationEditor.h"

#include "i18n.h"
#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"

#include "CommandEditor.h"

namespace ui {

namespace {
	const char* const WINDOW_TITLE = N_("Edit Conversation");
	const int MIN_HEIGHT_ACTORS_TREEVIEW = 160;
	const int MIN_HEIGHT_COMMAND_TREEVIEW = 200;

	enum {
		WIDGET_CONV_NAME_ENTRY,
		WIDGET_CONV_ACTOR_WITHIN_TALKDIST,
		WIDGET_CONV_ACTORS_ALWAYS_FACE,
		WIDGET_CONV_MAX_PLAY_COUNT_ENABLE,
		WIDGET_CONV_MAX_PLAY_COUNT_HBOX,
		WIDGET_CONV_MAX_PLAY_COUNT_ENTRY,
		WIDGET_ADD_ACTOR_BUTTON,
		WIDGET_DELETE_ACTOR_BUTTON,
		WIDGET_COMMAND_TREEVIEW,
		WIDGET_ADD_CMD_BUTTON,
		WIDGET_EDIT_CMD_BUTTON,
		WIDGET_MOVE_UP_CMD_BUTTON,
		WIDGET_MOVE_DOWN_CMD_BUTTON,
		WIDGET_DELETE_CMD_BUTTON,
	};

	inline std::string makeBold(const std::string& input)
	{
		return "<b>" + input + "</b>";
	}
}

ConversationEditor::ConversationEditor(const Glib::RefPtr<Gtk::Window>& parent, conversation::Conversation& conversation) :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), parent),
	_actorStore(gtk_list_store_new(2, 
								   G_TYPE_INT,		// actor number
								   G_TYPE_STRING)),	// display name
   _commandStore(gtk_list_store_new(4, 
								   G_TYPE_INT,		// cmd number
								   G_TYPE_STRING,	// actor name
								   G_TYPE_STRING,	// sentence
								   G_TYPE_STRING)),	// wait yes/no
   _conversation(conversation), // copy the conversation to a local object
   _targetConversation(conversation),
   _updateInProgress(false)
{
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);

	// Create the widgets
	populateWindow();

	// Load the conversation values into the widgets
	updateWidgets();

	// Clear the button sensitivity in the command actions panel
	updateCmdActionSensitivity(false);

	// Show and block
	show();
}

void ConversationEditor::populateWindow() {
	// Create the overall vbox
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Create the conversation properties pane
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel(makeBold(_("Properties"))), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(createPropertyPane(), 18, 1), FALSE, FALSE, 0);

	// Actors
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel(makeBold(_("Actors"))), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(createActorPanel(), 18, 1), FALSE, FALSE, 0);
	
	// Commands
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel(makeBold(_("Commands"))), FALSE, FALSE, 0);
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
					 gtkutil::LeftAlignedLabel(_("Name")),
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
							  gtkutil::LeftAlignedLabel(_("Actors must be within talk distance")), 
							  1, 2, row, row+1);
	
	row++;

	// Actors always face each other while talking
	_widgets[WIDGET_CONV_ACTORS_ALWAYS_FACE] = gtk_check_button_new();
	gtk_table_attach(GTK_TABLE(table), 
			gtkutil::RightAlignment(_widgets[WIDGET_CONV_ACTORS_ALWAYS_FACE]),
			0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), 
							  gtkutil::LeftAlignedLabel(_("Actors always face each other while talking")), 
							  1, 2, row, row+1);
	
	row++;

	// Max play count
	_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENABLE] = gtk_check_button_new();

	g_signal_connect(G_OBJECT(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENABLE]), "toggled", 
		G_CALLBACK(onMaxPlayCountEnabled), this);

	gtk_table_attach(GTK_TABLE(table), 
			gtkutil::RightAlignment(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENABLE]),
			0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);

	_widgets[WIDGET_CONV_MAX_PLAY_COUNT_HBOX] = gtk_hbox_new(FALSE, 6);
	GtkBox* hbox = GTK_BOX(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_HBOX]);

	_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENTRY] = gtk_spin_button_new_with_range(-1, 9999, 1);
	gtk_widget_set_size_request(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENTRY], 60, -1);

	gtk_box_pack_start(hbox, gtkutil::LeftAlignedLabel(_("Let this conversation play")), FALSE, FALSE, 0);
	gtk_box_pack_start(hbox, _widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENTRY], FALSE, FALSE, 0);
	gtk_box_pack_start(hbox, gtkutil::LeftAlignedLabel(_("times at maximum")), FALSE, FALSE, 0);

	gtk_table_attach_defaults(GTK_TABLE(table), 
							  _widgets[WIDGET_CONV_MAX_PLAY_COUNT_HBOX], 
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
	gtkutil::TextColumn actorColumn(_("Actor (click to edit)"), 1, false);
	
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
	_widgets[WIDGET_COMMAND_TREEVIEW] = tv;

	gtk_widget_set_size_request(tv, 300, MIN_HEIGHT_COMMAND_TREEVIEW);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), TRUE);

	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(onCommandSelectionChanged), this);
	
	// Key and value text columns
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn("#", 0, false));
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn(_("Actor"), 1));
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn(_("Command"), 2));
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtkutil::TextColumn(_("Wait"), 3));
	
	// Action buttons
	_widgets[WIDGET_ADD_CMD_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_ADD);
	_widgets[WIDGET_EDIT_CMD_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	_widgets[WIDGET_DELETE_CMD_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	_widgets[WIDGET_MOVE_UP_CMD_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	_widgets[WIDGET_MOVE_DOWN_CMD_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);

	g_signal_connect(G_OBJECT(_widgets[WIDGET_ADD_CMD_BUTTON]), "clicked", G_CALLBACK(onAddCommand), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_EDIT_CMD_BUTTON]), "clicked", G_CALLBACK(onEditCommand), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_MOVE_UP_CMD_BUTTON]), "clicked", G_CALLBACK(onMoveUpCommand), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_MOVE_DOWN_CMD_BUTTON]), "clicked", G_CALLBACK(onMoveDownCommand), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_DELETE_CMD_BUTTON]), "clicked", G_CALLBACK(onDeleteCommand), this);

	GtkWidget* actionVBox = gtk_vbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_ADD_CMD_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_EDIT_CMD_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_MOVE_UP_CMD_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_MOVE_DOWN_CMD_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_DELETE_CMD_BUTTON], FALSE, FALSE, 0);

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
	_updateInProgress = true;

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

	// Update the max play count
	if (_conversation.maxPlayCount != -1) {
		// Max play count is enabled
		gtk_widget_set_sensitive(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_HBOX], TRUE);

		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENTRY]), 
			_conversation.maxPlayCount
		);

		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENABLE]), 
			TRUE
		);
	}
	else {
		// Max play count disabled
		gtk_widget_set_sensitive(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_HBOX], FALSE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENTRY]), -1);

		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENABLE]), 
			FALSE
		);
	}

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

		std::string actorStr = (boost::format(_("Actor %d")) % cmd.actor).str();

		GtkTreeIter iter;
		gtk_list_store_append(_commandStore, &iter);
		gtk_list_store_set(_commandStore, &iter, 
						   0, i->first, 
						   1, actorStr.c_str(),
						   2, cmd.getSentence().c_str(),
						   3, cmd.waitUntilFinished ? _("yes") : _("no"),
						   -1);
	}

	_updateInProgress = false;
}

void ConversationEditor::selectCommand(int index) {
	// Select the actor passed from the command
	gtkutil::TreeModel::SelectionFinder finder(index, 0);

	gtk_tree_model_foreach(
		GTK_TREE_MODEL(_commandStore), 
		gtkutil::TreeModel::SelectionFinder::forEach, 
		&finder
	);
	
	// Select the found treeiter, if the name was found in the liststore
	if (finder.getPath() != NULL) {
		GtkTreeView* tv = GTK_TREE_VIEW(_widgets[WIDGET_COMMAND_TREEVIEW]);
		// Expand the treeview to display the target row
		gtk_tree_view_expand_to_path(tv, finder.getPath());
		// Highlight the target row
		gtk_tree_view_set_cursor(tv, finder.getPath(), NULL, false);
		// Make the selected row visible 
		gtk_tree_view_scroll_to_cell(tv, finder.getPath(), NULL, true, 0.3f, 0.0f);
	}
}

void ConversationEditor::moveSelectedCommand(int delta) {
	// Get the index of the currently selected command
	int index = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(_commandStore), &(_currentCommand), 0);

	int targetIndex = index + delta;
	
	if (targetIndex <= 0) {
		return; // can't move any more upwards
	}

	// Try to look up the command indices in the conversation
	conversation::Conversation::CommandMap::iterator oldCmd = _conversation.commands.find(index);
	conversation::Conversation::CommandMap::iterator newCmd = _conversation.commands.find(targetIndex);

	if (oldCmd != _conversation.commands.end() && newCmd != _conversation.commands.end()) {
		// There is a command at this position, swap it
		conversation::ConversationCommandPtr temp = newCmd->second;
		newCmd->second = oldCmd->second;
		oldCmd->second = temp;

		updateWidgets();

		// Select the moved command, for the user's convenience
		selectCommand(newCmd->first);
	}
}

void ConversationEditor::save() {
	// Name
	_conversation.name = gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_CONV_NAME_ENTRY]));

	_conversation.actorsMustBeWithinTalkdistance = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_CONV_ACTOR_WITHIN_TALKDIST])) ? true : false;

	_conversation.actorsAlwaysFaceEachOther = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_CONV_ACTORS_ALWAYS_FACE])) ? true : false;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENABLE]))) {
		_conversation.maxPlayCount = static_cast<int>(gtk_spin_button_get_value(
			GTK_SPIN_BUTTON(_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENTRY])
		));
	}
	else {
		_conversation.maxPlayCount = -1;
	}

	// Copy the working copy over the actual object
	_targetConversation = _conversation;
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
	if (self->_updateInProgress) return;

	// Get the selection
	bool hasSelection = gtk_tree_selection_get_selected(sel, NULL, &(self->_currentActor)) ? true : false;

	// Enable the delete buttons if we have a selection
	gtk_widget_set_sensitive(self->_widgets[WIDGET_DELETE_ACTOR_BUTTON], hasSelection ? TRUE : FALSE);
}

void ConversationEditor::updateCmdActionSensitivity(bool hasSelection) {
	// Enable the edit and delete buttons if we have a selection
	gtk_widget_set_sensitive(_widgets[WIDGET_EDIT_CMD_BUTTON], hasSelection ? TRUE : FALSE);
	gtk_widget_set_sensitive(_widgets[WIDGET_DELETE_CMD_BUTTON], hasSelection ? TRUE : FALSE);

	if (hasSelection) {
		// Check if this is the first command in the list, get the ID of the selected item
		int index = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(_commandStore), &(_currentCommand), 0);

		bool hasNext = _conversation.commands.find(index+1) != _conversation.commands.end();
		bool hasPrev = index > 1;

		gtk_widget_set_sensitive(_widgets[WIDGET_MOVE_UP_CMD_BUTTON], hasPrev ? TRUE : FALSE);
		gtk_widget_set_sensitive(_widgets[WIDGET_MOVE_DOWN_CMD_BUTTON], hasNext ? TRUE : FALSE);
	}
	else {
		gtk_widget_set_sensitive(_widgets[WIDGET_MOVE_UP_CMD_BUTTON], FALSE);
		gtk_widget_set_sensitive(_widgets[WIDGET_MOVE_DOWN_CMD_BUTTON], FALSE);
	}
}

void ConversationEditor::onCommandSelectionChanged(GtkTreeSelection* sel, ConversationEditor* self) {
	if (self->_updateInProgress) return;

	// Get the selection
	bool hasSelection = gtk_tree_selection_get_selected(sel, NULL, &(self->_currentCommand)) ? true : false;

	self->updateCmdActionSensitivity(hasSelection);
}

void ConversationEditor::onMaxPlayCountEnabled(GtkToggleButton* togglebutton, ConversationEditor* self) {
	if (self->_updateInProgress) return;

	if (gtk_toggle_button_get_active(togglebutton)) {
		// Enabled, write a new value in the spin button
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENTRY]), 1);

		gtk_widget_set_sensitive(self->_widgets[WIDGET_CONV_MAX_PLAY_COUNT_HBOX], TRUE);
	}
	else {
		// Disabled, write a -1 in the spin button
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->_widgets[WIDGET_CONV_MAX_PLAY_COUNT_ENTRY]), -1);

		gtk_widget_set_sensitive(self->_widgets[WIDGET_CONV_MAX_PLAY_COUNT_HBOX], FALSE);
	}
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
	self->_conversation.actors[idx] = _("New Actor");

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
	else {
		// Index not found, quit here
		return;
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

void ConversationEditor::onAddCommand(GtkWidget* w, ConversationEditor* self) {

	conversation::Conversation& conv = self->_conversation; // shortcut

	// Create a new command
	conversation::ConversationCommandPtr command(new conversation::ConversationCommand);

	// Construct a command editor (blocks on construction)
	CommandEditor editor(self->getRefPtr(), *command, conv);

	if (editor.getResult() == CommandEditor::RESULT_OK) {
		// The user hit ok, insert the command, find the first free index
		int index = 1;
		while (conv.commands.find(index) != conv.commands.end()) {
			index++;
		}

		// Insert the command at the new location
		conv.commands[index] = command;

		self->updateWidgets();
	}
}

void ConversationEditor::onEditCommand(GtkWidget* w, ConversationEditor* self) {
	// Get the index of the currently selected command
	int index = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(self->_commandStore), &(self->_currentCommand), 0);

	// Try to look up the command in the conversation
	conversation::Conversation::CommandMap::iterator i = self->_conversation.commands.find(index);

	if (i != self->_conversation.commands.end()) {
		// Get the command reference
		conversation::ConversationCommandPtr command = i->second;

		// Construct a command editor (blocks on construction)
		CommandEditor editor(self->getRefPtr(), *command, self->_conversation);

		if (editor.getResult() == CommandEditor::RESULT_OK) {
			self->updateWidgets();
		}
	}
}

void ConversationEditor::onMoveUpCommand(GtkWidget* w, ConversationEditor* self) {
	// Pass the call
	self->moveSelectedCommand(-1);
}

void ConversationEditor::onMoveDownCommand(GtkWidget* w, ConversationEditor* self) {
	// Pass the call
	self->moveSelectedCommand(+1);
}

void ConversationEditor::onDeleteCommand(GtkWidget* w, ConversationEditor* self) {
	// Get the index of the currently selected command
	int index = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(self->_commandStore), &(self->_currentCommand), 0);

	// Add the new command to the map
	conversation::Conversation::CommandMap::iterator i = self->_conversation.commands.find(index);

	if (i != self->_conversation.commands.end()) {
		// Remove the specified command
		self->_conversation.commands.erase(index);
	}
	else {
		// Index not found, quit here
		return;
	}

	// Adjust the numbers of all other commands with higher numbers
	while (self->_conversation.commands.find(index + 1) != self->_conversation.commands.end()) {
		// Move the commands with the higher index "down" by one number...
		self->_conversation.commands[index] = self->_conversation.commands[index + 1];
		// ...and remove it from the old location
		self->_conversation.commands.erase(index + 1);

		index++;
	}
	
	// Update the widgets
	self->updateWidgets();
}

} // namespace ui
