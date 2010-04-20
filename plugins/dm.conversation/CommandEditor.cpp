#include "CommandEditor.h"

#include "i18n.h"
#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"

#include <boost/format.hpp>
#include "itextstream.h"

#include "ConversationCommandLibrary.h"

namespace ui {

	namespace {
		const char* const WINDOW_TITLE = N_("Edit Command");
	}

CommandEditor::CommandEditor(GtkWindow* parent, conversation::ConversationCommand& command, conversation::Conversation conv) :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), parent),
	_conversation(conv),
	_command(command), // copy the conversation command to a local object
	_targetCommand(command),
	_result(NUM_RESULTS),
	_actorStore(gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING)), // number + caption
	_commandStore(gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING)), // number + caption
	_argTable(NULL),
	_argumentWidget(NULL)
{
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);

	// Fill the actor store
	for (conversation::Conversation::ActorMap::const_iterator i = _conversation.actors.begin();
		 i != _conversation.actors.end(); ++i)
	{
		std::string actorStr = (boost::format(_("Actor %d (%s)")) % i->first % i->second).str();

		GtkTreeIter iter;
		gtk_list_store_append(_actorStore, &iter);
		gtk_list_store_set(_actorStore, &iter, 
						   0, i->first, 
						   1, actorStr.c_str(),
						   -1);
	}

	// Let the command library fill the command store
	conversation::ConversationCommandLibrary::Instance().populateListStore(_commandStore);

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

	// Select the type passed from the command
	gtkutil::TreeModel::SelectionFinder cmdFinder(_command.type, 0);

	gtk_tree_model_foreach(
		GTK_TREE_MODEL(_commandStore), 
		gtkutil::TreeModel::SelectionFinder::forEach, 
		&cmdFinder
	);
	
	// Select the found treeiter, if the name was found in the liststore
	if (cmdFinder.getPath() != NULL) {
		GtkTreeIter iter = cmdFinder.getIter();
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(_commandDropDown), &iter);
	}

	// Populate the correct command argument widgets
	createArgumentWidgets(_command.type);

	// Pre-fill the values
	for (conversation::ConversationCommand::ArgumentMap::const_iterator i = _command.arguments.begin();
		i != _command.arguments.end(); ++i)
	{
		int argIndex = i->first;

		if (argIndex > static_cast<int>(_argumentItems.size()) || argIndex < 0)
		{
			// Invalid command argument index
			globalErrorStream() << "Invalid command argument index " << argIndex << std::endl;
			continue;
		}

		// Load the value into the argument item
		_argumentItems[argIndex - 1]->setValueFromString(i->second);
	}

	// Update the "wait until finished" flag 
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_waitUntilFinished), 
		_command.waitUntilFinished ? TRUE : FALSE
	);

	// Update the sensitivity of the correct flag
	upateWaitUntilFinished(_command.type);
}

void CommandEditor::save() {
	// Get the active actor item
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(_actorDropDown), &iter)) {
		// Get the model
		GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(_actorDropDown));
		// Get the value
		_command.actor = gtkutil::TreeModel::getInt(model, &iter, 0);
	}

	// Get the active command type selection
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(_commandDropDown), &iter)) {
		// Get the model
		GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(_commandDropDown));
		// Get the value
		_command.type = gtkutil::TreeModel::getInt(model, &iter, 0);
	}

	// Clear the existing arguments
	_command.arguments.clear();

	int index = 1;

	for (ArgumentItemList::iterator i = _argumentItems.begin();
		 i != _argumentItems.end(); ++i, ++index)
	{
		_command.arguments[index] = (*i)->getValue();
	}

	// Get the value of the "wait until finished" flag
	try {
		const conversation::ConversationCommandInfo& cmdInfo = 
			conversation::ConversationCommandLibrary::Instance().findCommandInfo(_command.type);

		if (cmdInfo.waitUntilFinishedAllowed) {
			// Load the value
			_command.waitUntilFinished = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(_waitUntilFinished)) ? true : false;
		}
		else {
			// Command doesn't support "wait until finished", set to default == true
			_command.waitUntilFinished = true;
		}
	}
	catch (std::runtime_error e) {
		globalErrorStream() << "Cannot find conversation command info for index " << _command.type << std::endl;
	}

	// Copy the command over the target object
	_targetCommand = _command;
}

void CommandEditor::populateWindow() {
	// Create the overall vbox
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Actor
	gtk_box_pack_start(GTK_BOX(vbox), 
		gtkutil::LeftAlignedLabel(std::string("<b>") + _("Actor") + "</b>"), 
		FALSE, FALSE, 0);

	// Create the actor dropdown box
	_actorDropDown = gtk_combo_box_new_with_model(GTK_TREE_MODEL(_actorStore));

	// Add the cellrenderer for the name
	GtkCellRenderer* nameRenderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_actorDropDown), nameRenderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_actorDropDown), nameRenderer, "text", 1);
	
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(_actorDropDown, 18, 1), FALSE, FALSE, 0);

	// Command Type
	gtk_box_pack_start(GTK_BOX(vbox), 
		gtkutil::LeftAlignedLabel(std::string("<b>") + _("Command") + "</b>"), 
		FALSE, FALSE, 0);
	_commandDropDown = gtk_combo_box_new_with_model(GTK_TREE_MODEL(_commandStore));

	// Connect the signal to get notified of further changes
	g_signal_connect(G_OBJECT(_commandDropDown), "changed", G_CALLBACK(onCommandTypeChange) , this);

	// Add the cellrenderer for the name
	GtkCellRenderer* cmdNameRenderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_commandDropDown), cmdNameRenderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_commandDropDown), cmdNameRenderer, "text", 1);

	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(_commandDropDown, 18, 1), FALSE, FALSE, 0);
	
	// Command Arguments
	gtk_box_pack_start(GTK_BOX(vbox), 
		gtkutil::LeftAlignedLabel(std::string("<b>") + _("Command Arguments") + "</b>"), 
		FALSE, FALSE, 0);
	
	// Create the alignment container that hold the (exchangable) widget table
	_argAlignment = gtk_alignment_new(0.0, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(_argAlignment), 0, 0, 18, 0);

	gtk_box_pack_start(GTK_BOX(vbox), _argAlignment, FALSE, FALSE, 3);

	// Wait until finished
	gtk_box_pack_start(GTK_BOX(vbox), 
		gtkutil::LeftAlignedLabel(std::string("<b>") + _("Command Properties") + "</b>"), 
		FALSE, FALSE, 0);

	_waitUntilFinished = gtk_check_button_new_with_label(_("Wait until finished"));
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(_waitUntilFinished, 18, 1), FALSE, FALSE, 0);

	// Buttons
	gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);
}

void CommandEditor::commandTypeChanged() {
	int newCommandTypeID = -1;
	
	// Get the currently selected effect name from the combo box
	GtkTreeIter iter;
	GtkComboBox* combo = GTK_COMBO_BOX(_commandDropDown);
	
	if (gtk_combo_box_get_active_iter(combo, &iter)) {
		GtkTreeModel* model = gtk_combo_box_get_model(combo);
		
		newCommandTypeID = gtkutil::TreeModel::getInt(model, &iter, 0);
	}

	// Create the argument widgets for this new command type
	createArgumentWidgets(newCommandTypeID);

	// Update the sensitivity of the correct flag
	upateWaitUntilFinished(newCommandTypeID);
}

void CommandEditor::upateWaitUntilFinished(int commandTypeID) {
	// Update the sensitivity of the correct flag
	try {
		const conversation::ConversationCommandInfo& cmdInfo = 
			conversation::ConversationCommandLibrary::Instance().findCommandInfo(commandTypeID);

		gtk_widget_set_sensitive(_waitUntilFinished, cmdInfo.waitUntilFinishedAllowed ? TRUE : FALSE);
	}
	catch (std::runtime_error& e) {
		globalErrorStream() << "Cannot find conversation command info for index " << commandTypeID << std::endl;
	}
}

void CommandEditor::createArgumentWidgets(int commandTypeID) {

	try {
		const conversation::ConversationCommandInfo& cmdInfo = 
			conversation::ConversationCommandLibrary::Instance().findCommandInfo(commandTypeID);

		// Remove all possible previous items from the list
		_argumentItems.clear();

		// Remove the old widget if there exists one
		if (_argumentWidget != NULL) {
			// This removes the old table from the alignment container
			// greebo: Increase the refCount of the widget to prevent destruction.
			// Destruction would cause weird shutdown crashes.
			g_object_ref(G_OBJECT(_argumentWidget));
			gtk_container_remove(GTK_CONTAINER(_argAlignment), _argumentWidget);
		}

		if (cmdInfo.arguments.empty()) {
			// No arguments, just push an empty label into the alignment
			GtkWidget* label = gtkutil::LeftAlignedLabel(
				std::string("<i>") + _("None") + "</i>");
			gtk_container_add(GTK_CONTAINER(_argAlignment), label);

			gtk_widget_show_all(label);

			// Remember this widget
			_argumentWidget = label;

			return;
		}
		
		// Create the tooltips group for the help mouseover texts
		_tooltips = gtk_tooltips_new();
		gtk_tooltips_enable(_tooltips);
		
		// Setup the table with default spacings
		_argTable = gtk_table_new(static_cast<guint>(cmdInfo.arguments.size()), 3, false);
		gtk_table_set_col_spacings(GTK_TABLE(_argTable), 12);
		gtk_table_set_row_spacings(GTK_TABLE(_argTable), 6);
		gtk_container_add(GTK_CONTAINER(_argAlignment), _argTable); 
		_argumentWidget = _argTable;

		typedef conversation::ConversationCommandInfo::ArgumentInfoList::const_iterator ArgumentIter;

		int index = 1;

		for (ArgumentIter i = cmdInfo.arguments.begin(); 
			 i != cmdInfo.arguments.end(); ++i, ++index)
		{
			const conversation::ArgumentInfo& argInfo = *i;

			CommandArgumentItemPtr item;
			
			switch (argInfo.type)
			{
			case conversation::ArgumentInfo::ARGTYPE_BOOL:
				// Create a new bool argument item
				item = CommandArgumentItemPtr(new BooleanArgument(argInfo, _tooltips));
				break;
			case conversation::ArgumentInfo::ARGTYPE_INT:
			case conversation::ArgumentInfo::ARGTYPE_FLOAT:
			case conversation::ArgumentInfo::ARGTYPE_STRING:
				// Create a new string argument item
				item = CommandArgumentItemPtr(new StringArgument(argInfo, _tooltips));
				break;
			case conversation::ArgumentInfo::ARGTYPE_VECTOR:
			case conversation::ArgumentInfo::ARGTYPE_SOUNDSHADER:
				// Create a new string argument item
				item = CommandArgumentItemPtr(new StringArgument(argInfo, _tooltips));
				break;
			case conversation::ArgumentInfo::ARGTYPE_ACTOR:
				// Create a new actor argument item
				item = CommandArgumentItemPtr(new ActorArgument(argInfo, _tooltips, _actorStore));
				break;
			case conversation::ArgumentInfo::ARGTYPE_ENTITY:
				// Create a new string argument item
				item = CommandArgumentItemPtr(new StringArgument(argInfo, _tooltips));
				break;
			default:
				globalErrorStream() << "Unknown command argument type: " << argInfo.type << std::endl;
				break;
			};

			if (item != NULL) {
				_argumentItems.push_back(item);
				
				if (argInfo.type != conversation::ArgumentInfo::ARGTYPE_BOOL) {
					// The label
					gtk_table_attach(
						GTK_TABLE(_argTable), item->getLabelWidget(),
						0, 1, index-1, index, // index starts with 1, hence the -1
						GTK_FILL, (GtkAttachOptions)0, 0, 0
					);
					
					// The edit widgets
					gtk_table_attach_defaults(
						GTK_TABLE(_argTable), item->getEditWidget(),
						1, 2, index-1, index // index starts with 1, hence the -1
					);
				}
				else {
					// This is a checkbutton - should be spanned over two columns
					gtk_table_attach(
						GTK_TABLE(_argTable), item->getEditWidget(),
						0, 2, index-1, index, // index starts with 1, hence the -1
						GTK_FILL, (GtkAttachOptions)0, 0, 0
					);
				}
				
				// The help widgets
				gtk_table_attach(
					GTK_TABLE(_argTable), item->getHelpWidget(),
					2, 3, index-1, index, // index starts with 1, hence the -1
					(GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0
				);
			}
		}
		
		// Show the table and all subwidgets
		gtk_widget_show_all(_argTable);
	}
	catch (std::runtime_error e) {
		globalErrorStream() << "Cannot find conversation command info for index " << commandTypeID << std::endl;
	}
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

void CommandEditor::onCommandTypeChange(GtkWidget* combobox, CommandEditor* self) {
	self->commandTypeChanged();
}

} // namespace ui
