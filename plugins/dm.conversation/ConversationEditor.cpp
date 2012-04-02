#include "ConversationEditor.h"

#include "i18n.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"

#include <gtkmm/box.h>
#include <gtkmm/table.h>
#include <gtkmm/entry.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/treeview.h>
#include <gtkmm/stock.h>

#include <boost/format.hpp>

#include "CommandEditor.h"

namespace ui {

namespace
{
	const char* const WINDOW_TITLE = N_("Edit Conversation");
	const int MIN_HEIGHT_ACTORS_TREEVIEW = 160;
	const int MIN_HEIGHT_COMMAND_TREEVIEW = 200;

	inline std::string makeBold(const std::string& input)
	{
		return "<b>" + input + "</b>";
	}
}

ConversationEditor::ConversationEditor(const Glib::RefPtr<Gtk::Window>& parent, conversation::Conversation& conversation) :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), parent),
	_actorStore(Gtk::ListStore::create(_actorColumns)),
	_commandStore(Gtk::ListStore::create(_commandColumns)),
   _conversation(conversation), // copy the conversation to a local object
   _targetConversation(conversation),
   _updateInProgress(false)
{
	set_border_width(12);

	// Create the widgets
	populateWindow();

	// Load the conversation values into the widgets
	updateWidgets();

	// Clear the button sensitivity in the command actions panel
	updateCmdActionSensitivity(false);

	// Show and block
	show();
}

void ConversationEditor::populateWindow()
{
	// Create the overall vbox
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));

	// Create the conversation properties pane
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Properties")))), false, false, 0);
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(createPropertyPane(), 18, 1)), false, false, 0);

	// Actors
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Actors")))), false, false, 0);
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(createActorPanel(), 18, 1)), false, false, 0);

	// Commands
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Commands")))), false, false, 0);
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(createCommandPanel(), 18, 1)), true, true, 0);

	// Buttons
	vbox->pack_start(createButtonPanel(), false, false, 0);

	add(*vbox);
}

Gtk::Widget& ConversationEditor::createPropertyPane()
{
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));

	// Table for entry boxes
	Gtk::Table* table = Gtk::manage(new Gtk::Table(4, 2, false));
	table->set_row_spacings(6);
	table->set_col_spacings(12);

	vbox->pack_start(*table, false, false, 0);

	int row = 0;

	// Conversation name
	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Name"))),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);

	_convNameEntry = Gtk::manage(new Gtk::Entry);
	table->attach(*_convNameEntry, 1, 2, row, row+1);

	row++;

	// Actors within talk distance
	_convActorsWithinTalkDistance = Gtk::manage(new Gtk::CheckButton);
	table->attach(*Gtk::manage(new gtkutil::RightAlignment(*_convActorsWithinTalkDistance)),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Actors must be within talk distance"))),
				  1, 2, row, row+1);

	row++;

	// Actors always face each other while talking
	_convActorsAlwaysFace = Gtk::manage(new Gtk::CheckButton);
	table->attach(*Gtk::manage(new gtkutil::RightAlignment(*_convActorsAlwaysFace)),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Actors always face each other while talking"))),
				  1, 2, row, row+1);

	row++;

	// Max play count
	_convMaxPlayCountEnable = Gtk::manage(new Gtk::CheckButton);
	_convMaxPlayCountEnable->signal_toggled().connect(sigc::mem_fun(*this, &ConversationEditor::onMaxPlayCountEnabled));

	table->attach(*Gtk::manage(new gtkutil::RightAlignment(*_convMaxPlayCountEnable)),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);

	_maxPlayCountHBox = Gtk::manage(new Gtk::HBox(false, 6));

	Gtk::Adjustment* adj = Gtk::manage(new Gtk::Adjustment(-1, -1, 9999));
	_maxPlayCount = Gtk::manage(new Gtk::SpinButton(*adj, 1, 0));
	_maxPlayCount->set_size_request(60, -1);

	_maxPlayCountHBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Let this conversation play"))), false, false, 0);
	_maxPlayCountHBox->pack_start(*_maxPlayCount, false, false, 0);
	_maxPlayCountHBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("times at maximum"))), false, false, 0);

	table->attach(*_maxPlayCountHBox, 1, 2, row, row+1);

	row++;

	return *vbox;
}

Gtk::Widget& ConversationEditor::createActorPanel()
{
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	// Tree view
	_actorView = Gtk::manage(new Gtk::TreeView(_actorStore));
	_actorView->set_size_request(-1, MIN_HEIGHT_ACTORS_TREEVIEW);
	_actorView->set_headers_visible(true);
	_actorView->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &ConversationEditor::onActorSelectionChanged));

	// Key and value text columns
	_actorView->append_column(*Gtk::manage(new gtkutil::TextColumn("#", _actorColumns.actorNumber, false)));

	// Construct a new editable text column
	gtkutil::TextColumn* actorColumn = Gtk::manage(new gtkutil::TextColumn(_("Actor (click to edit)"), _actorColumns.displayName, false));

	Gtk::CellRendererText* rend = actorColumn->getCellRenderer();
	rend->property_editable() = true;
	rend->signal_edited().connect(sigc::mem_fun(*this, &ConversationEditor::onActorEdited));

	// Cast the column object to a GtkTreeViewColumn* and append it
	_actorView->append_column(*actorColumn);

	// Action buttons
	_addActorButton = Gtk::manage(new Gtk::Button(Gtk::Stock::ADD));
	_delActorButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
	_addActorButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationEditor::onAddActor));
	_delActorButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationEditor::onDeleteActor));

	Gtk::VBox* actionVBox = Gtk::manage(new Gtk::VBox(false, 6));

	actionVBox->pack_start(*_addActorButton, false, false, 0);
	actionVBox->pack_start(*_delActorButton, false, false, 0);

	// Actors treeview goes left, actionbuttons go right
	hbox->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_actorView)), true, true, 0);
	hbox->pack_start(*actionVBox, false, false, 0);

	return *hbox;
}

Gtk::Widget& ConversationEditor::createCommandPanel()
{
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	// Tree view
	_commandView = Gtk::manage(new Gtk::TreeView(_commandStore));
	_commandView->set_size_request(300, MIN_HEIGHT_COMMAND_TREEVIEW);
	_commandView->set_headers_visible(true);
	_commandView->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &ConversationEditor::onCommandSelectionChanged));

	// Key and value text columns
	_commandView->append_column(*Gtk::manage(new gtkutil::TextColumn("#", _commandColumns.cmdNumber, false)));
	_commandView->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Actor"), _commandColumns.actorName)));
	_commandView->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Command"), _commandColumns.sentence)));
	_commandView->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Wait"), _commandColumns.wait)));

	// Action buttons
	_addCmdButton = Gtk::manage(new Gtk::Button(Gtk::Stock::ADD));
	_editCmdButton = Gtk::manage(new Gtk::Button(Gtk::Stock::EDIT));
	_delCmdButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
	_moveUpCmdButton = Gtk::manage(new Gtk::Button(Gtk::Stock::GO_UP));
	_moveDownCmdButton = Gtk::manage(new Gtk::Button(Gtk::Stock::GO_DOWN));

	_addCmdButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationEditor::onAddCommand));
	_editCmdButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationEditor::onEditCommand));
	_moveUpCmdButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationEditor::onMoveUpCommand));
	_moveDownCmdButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationEditor::onMoveDownCommand));
	_delCmdButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationEditor::onDeleteCommand));

	Gtk::VBox* actionVBox = Gtk::manage(new Gtk::VBox(false, 6));

	actionVBox->pack_start(*_addCmdButton, false, false, 0);
	actionVBox->pack_start(*_editCmdButton, false, false, 0);
	actionVBox->pack_start(*_moveUpCmdButton, false, false, 0);
	actionVBox->pack_start(*_moveDownCmdButton, false, false, 0);
	actionVBox->pack_start(*_delCmdButton, false, false, 0);

	// Command treeview goes left, action buttons go right
	hbox->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_commandView)), true, true, 0);
	hbox->pack_start(*actionVBox, false, false, 0);

	return *hbox;
}

Gtk::Widget& ConversationEditor::createButtonPanel()
{
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(true, 12));

	// Save button
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationEditor::onSave));
	buttonHBox->pack_end(*okButton, true, true, 0);

	// Cancel Button
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &ConversationEditor::onCancel));
	buttonHBox->pack_end(*cancelButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*buttonHBox));
}

void ConversationEditor::updateWidgets()
{
	_updateInProgress = true;

	// Clear the liststores first
	_actorStore->clear();
	_commandStore->clear();

	_currentActor = Gtk::TreeModel::iterator();
	_currentCommand = Gtk::TreeModel::iterator();

	updateCmdActionSensitivity(false);
	_delActorButton->set_sensitive(false);

	// Name
	_convNameEntry->set_text(_conversation.name);

	_convActorsWithinTalkDistance->set_active(_conversation.actorsMustBeWithinTalkdistance);
	_convActorsAlwaysFace->set_active(_conversation.actorsAlwaysFaceEachOther);

	// Update the max play count
	if (_conversation.maxPlayCount != -1)
	{
		// Max play count is enabled
		_maxPlayCountHBox->set_sensitive(true);
		_maxPlayCount->set_value(_conversation.maxPlayCount);
		_convMaxPlayCountEnable->set_active(true);
	}
	else
	{
		// Max play count disabled
		_maxPlayCountHBox->set_sensitive(false);
		_maxPlayCount->set_value(-1);
		_convMaxPlayCountEnable->set_active(false);
	}

	// Actors
	for (conversation::Conversation::ActorMap::const_iterator i = _conversation.actors.begin();
		 i != _conversation.actors.end(); ++i)
	{
		Gtk::TreeModel::Row row = *_actorStore->append();

		row[_actorColumns.actorNumber] = i->first;
		row[_actorColumns.displayName] = i->second;
	}

	// Commands
	for (conversation::Conversation::CommandMap::const_iterator i = _conversation.commands.begin();
		 i != _conversation.commands.end(); ++i)
	{
		const conversation::ConversationCommand& cmd = *(i->second);

		Gtk::TreeModel::Row row = *_commandStore->append();

		row[_commandColumns.cmdNumber] = i->first;
		row[_commandColumns.actorName] = (boost::format(_("Actor %d")) % cmd.actor).str();
		row[_commandColumns.sentence] = cmd.getSentence();
		row[_commandColumns.wait] = cmd.waitUntilFinished ? _("yes") : _("no");
	}

	_updateInProgress = false;
}

void ConversationEditor::selectCommand(int index)
{
	// Select the actor passed from the command
	gtkutil::TreeModel::findAndSelectInteger(_commandView, index, _commandColumns.cmdNumber);
}

void ConversationEditor::moveSelectedCommand(int delta)
{
	// Get the index of the currently selected command
	int index = (*_currentCommand)[_commandColumns.cmdNumber];

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

void ConversationEditor::save()
{
	// Name
	_conversation.name = _convNameEntry->get_text();

	_conversation.actorsMustBeWithinTalkdistance = _convActorsWithinTalkDistance->get_active();
	_conversation.actorsAlwaysFaceEachOther = _convActorsAlwaysFace->get_active();

	if (_convMaxPlayCountEnable->get_active())
	{
		_conversation.maxPlayCount = _maxPlayCount->get_value_as_int();
	}
	else
	{
		_conversation.maxPlayCount = -1;
	}

	// Copy the working copy over the actual object
	_targetConversation = _conversation;
}

void ConversationEditor::onSave()
{
	// First, save to the conversation object
	save();

	// Then close the window
	destroy();
}

void ConversationEditor::onCancel()
{
	// Just close the window without writing the values
	destroy();
}

void ConversationEditor::onActorSelectionChanged()
{
	if (_updateInProgress) return;

	// Get the selection
	_currentActor = _actorView->get_selection()->get_selected();

	// Enable the delete buttons if we have a selection
	_delActorButton->set_sensitive(_currentActor ? true : false);
}

void ConversationEditor::updateCmdActionSensitivity(bool hasSelection)
{
	// Enable the edit and delete buttons if we have a selection
	_editCmdButton->set_sensitive(hasSelection);
	_delCmdButton->set_sensitive(hasSelection);

	if (hasSelection)
	{
		// Check if this is the first command in the list, get the ID of the selected item
		int index = (*_currentCommand)[_commandColumns.cmdNumber];

		bool hasNext = _conversation.commands.find(index+1) != _conversation.commands.end();
		bool hasPrev = index > 1;

		_moveUpCmdButton->set_sensitive(hasPrev);
		_moveDownCmdButton->set_sensitive(hasNext);
	}
	else
	{
		_moveUpCmdButton->set_sensitive(false);
		_moveDownCmdButton->set_sensitive(false);
	}
}

void ConversationEditor::onCommandSelectionChanged()
{
	if (_updateInProgress) return;

	// Get the selection
	_currentCommand = _commandView->get_selection()->get_selected();

	updateCmdActionSensitivity(_currentCommand ? true : false);
}

void ConversationEditor::onMaxPlayCountEnabled()
{
	if (_updateInProgress) return;

	if (_convMaxPlayCountEnable->get_active())
	{
		// Enabled, write a new value in the spin button
		_maxPlayCount->set_value(1);

		_maxPlayCountHBox->set_sensitive(true);
	}
	else
	{
		// Disabled, write a -1 in the spin button
		_maxPlayCount->set_value(-1);

		_maxPlayCountHBox->set_sensitive(false);
	}
}

void ConversationEditor::onAddActor()
{
	// Get the lowest available actor ID
	int idx = 1;

	for (idx = 1; idx < INT_MAX; idx++)
	{
		if (_conversation.actors.find(idx) == _conversation.actors.end())
		{
			break;
		}
	}

	// Add the new actor to the map
	_conversation.actors[idx] = _("New Actor");

	// Update the widgets
	updateWidgets();
}

void ConversationEditor::onDeleteActor()
{
	// Get the index of the currently selected actor
	int index = (*_currentActor)[_actorColumns.actorNumber];

	// Add the new actor to the map
	conversation::Conversation::ActorMap::iterator i = _conversation.actors.find(index);

	if (i != _conversation.actors.end()) {
		// Remove the specified actor
		_conversation.actors.erase(index);
	}
	else {
		// Index not found, quit here
		return;
	}

	// Adjust the numbers of all other actors with higher numbers
	while (_conversation.actors.find(index + 1) != _conversation.actors.end()) {
		// Move the actor with the higher index "down" by one number...
		_conversation.actors[index] = _conversation.actors[index + 1];
		// ...and remove it from the old location
		_conversation.actors.erase(index + 1);

		index++;
	}

	// Update the widgets
	updateWidgets();
}

void ConversationEditor::onActorEdited(const Glib::ustring& path, const Glib::ustring& new_text)
{
	Gtk::TreeModel::iterator iter = _actorStore->get_iter(path);

	if (iter)
	{
		// The iter points to the edited cell now, get the actor number
		int actorNum = (*iter)[_actorColumns.actorNumber];

		// Update the conversation
		_conversation.actors[actorNum] = new_text;

		// Update all widgets
		updateWidgets();
	}
}

void ConversationEditor::onAddCommand()
{
	conversation::Conversation& conv = _conversation; // shortcut

	// Create a new command
	conversation::ConversationCommandPtr command(new conversation::ConversationCommand);

	// Construct a command editor (blocks on construction)
	CommandEditor editor(getRefPtr(), *command, conv);

	if (editor.getResult() == CommandEditor::RESULT_OK)
	{
		// The user hit ok, insert the command, find the first free index
		int index = 1;
		while (conv.commands.find(index) != conv.commands.end()) {
			index++;
		}

		// Insert the command at the new location
		conv.commands[index] = command;

		updateWidgets();
	}
}

void ConversationEditor::onEditCommand()
{
	// Get the index of the currently selected command
	int index = (*_currentCommand)[_commandColumns.cmdNumber];

	// Try to look up the command in the conversation
	conversation::Conversation::CommandMap::iterator i = _conversation.commands.find(index);

	if (i != _conversation.commands.end())
	{
		// Get the command reference
		conversation::ConversationCommandPtr command = i->second;

		// Construct a command editor (blocks on construction)
		CommandEditor editor(getRefPtr(), *command, _conversation);

		if (editor.getResult() == CommandEditor::RESULT_OK) {
			updateWidgets();
		}
	}
}

void ConversationEditor::onMoveUpCommand()
{
	// Pass the call
	moveSelectedCommand(-1);
}

void ConversationEditor::onMoveDownCommand()
{
	// Pass the call
	moveSelectedCommand(+1);
}

void ConversationEditor::onDeleteCommand()
{
	// Get the index of the currently selected command
	int index = (*_currentCommand)[_commandColumns.cmdNumber];

	// Add the new command to the map
	conversation::Conversation::CommandMap::iterator i = _conversation.commands.find(index);

	if (i != _conversation.commands.end()) {
		// Remove the specified command
		_conversation.commands.erase(index);
	}
	else {
		// Index not found, quit here
		return;
	}

	// Adjust the numbers of all other commands with higher numbers
	while (_conversation.commands.find(index + 1) != _conversation.commands.end()) {
		// Move the commands with the higher index "down" by one number...
		_conversation.commands[index] = _conversation.commands[index + 1];
		// ...and remove it from the old location
		_conversation.commands.erase(index + 1);

		index++;
	}

	// Update the widgets
	updateWidgets();
}

} // namespace ui
