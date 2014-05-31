#include "ConversationEditor.h"

#include "i18n.h"
#include "string/string.h"

#include <boost/format.hpp>
#include <boost/regex.hpp>

#include "CommandEditor.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>

namespace ui 
{

namespace
{
	const char* const WINDOW_TITLE = N_("Edit Conversation");
}

ConversationEditor::ConversationEditor(wxWindow* parent, conversation::Conversation& conversation) :
	DialogBase(_(WINDOW_TITLE), parent),
	_actorStore(new wxutil::TreeModel(_actorColumns, true)),
	_commandStore(new wxutil::TreeModel(_commandColumns, true)),
   _conversation(conversation), // copy the conversation to a local object
   _targetConversation(conversation),
   _updateInProgress(false)
{
	// Create the widgets
	populateWindow();

	// Load the conversation values into the widgets
	updateWidgets();

	// Clear the button sensitivity in the command actions panel
	updateCmdActionSensitivity(false);

	SetSize(500, 680);
}

void ConversationEditor::populateWindow()
{
	loadNamedPanel(this, "ConvEditorMainPanel");

	makeLabelBold(this, "ConvEditorPropertyLabel");
	makeLabelBold(this, "ConvEditorActorLabel");
	makeLabelBold(this, "ConvEditorCommandLabel");

	findNamedObject<wxCheckBox>(this, "ConvEditorRepeatCheckbox")->Connect(
		wxEVT_CHECKBOX, wxCommandEventHandler(ConversationEditor::onMaxPlayCountEnabled), NULL, this);
	
	// Actor Panel
	wxPanel* actorPanel = findNamedObject<wxPanel>(this, "ConvEditorActorPanel");

	_actorView = wxutil::TreeView::CreateWithModel(actorPanel, _actorStore);
	_actorView->SetSize(wxSize(350, 160));
	actorPanel->GetSizer()->Add(_actorView, 1, wxEXPAND);

	_actorView->AppendTextColumn("#", _actorColumns.actorNumber.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
	_actorView->AppendTextColumn(_("Actor (click to edit)"), _actorColumns.displayName.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_actorView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(ConversationEditor::onActorSelectionChanged), NULL, this);
	_actorView->Connect(wxEVT_DATAVIEW_ITEM_EDITING_DONE, 
		wxDataViewEventHandler(ConversationEditor::onActorEdited), NULL, this);

	// Wire up button signals
	_addActorButton = findNamedObject<wxButton>(this, "ConvEditorAddActorButton");
	_addActorButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationEditor::onAddActor), NULL, this);

	_delActorButton = findNamedObject<wxButton>(this, "ConvEditorDeleteActorButton");
	_delActorButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationEditor::onDeleteActor), NULL, this);
	_delActorButton->Enable(false);

	// Command Panel
	wxPanel* commandPanel = findNamedObject<wxPanel>(this, "ConvEditorCommandPanel");
	
	_commandView = wxutil::TreeView::CreateWithModel(commandPanel, _commandStore);
	_commandView->SetSize(wxSize(350, 200));
	commandPanel->GetSizer()->Add(_commandView, 1, wxEXPAND);

	_commandView->AppendTextColumn("#", _commandColumns.cmdNumber.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
	_commandView->AppendTextColumn(_("Actor"), _commandColumns.actorName.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
	_commandView->AppendTextColumn(_("Command"), _commandColumns.sentence.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
	_commandView->AppendTextColumn(_("Wait"), _commandColumns.wait.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	_commandView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(ConversationEditor::onCommandSelectionChanged), NULL, this);

	// Wire up buttons
	_addCmdButton = findNamedObject<wxButton>(this, "ConvEditorAddCommandButton");
	_addCmdButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationEditor::onAddCommand), NULL, this);

	_delCmdButton = findNamedObject<wxButton>(this, "ConvEditorDeleteCommandButton");
	_delCmdButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationEditor::onDeleteCommand), NULL, this);
	_delCmdButton->Enable(false);

	_editCmdButton = findNamedObject<wxButton>(this, "ConvEditorEditCommandButton");
	_editCmdButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationEditor::onEditCommand), NULL, this);
	_editCmdButton->Enable(false);

	_moveUpCmdButton = findNamedObject<wxButton>(this, "ConvEditorMoveUpCommandButton");
	_moveUpCmdButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationEditor::onMoveUpCommand), NULL, this);
	_moveUpCmdButton->Enable(false);

	_moveDownCmdButton = findNamedObject<wxButton>(this, "ConvEditorMoveDownCommandButton");
	_moveDownCmdButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ConversationEditor::onMoveDownCommand), NULL, this);
	_moveDownCmdButton->Enable(false);

	// Dialog buttons
	findNamedObject<wxButton>(this, "ConvEditorCancelButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ConversationEditor::onCancel), NULL, this);
	findNamedObject<wxButton>(this, "ConvEditorOkButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ConversationEditor::onSave), NULL, this);
}

void ConversationEditor::updateWidgets()
{
	_updateInProgress = true;

	// Clear the liststores first
	_actorStore->Clear();
	_commandStore->Clear();

	_currentActor = wxDataViewItem();
	_currentCommand = wxDataViewItem();

	updateCmdActionSensitivity(false);
	_delActorButton->Enable(false);

	// Name
	findNamedObject<wxTextCtrl>(this, "ConvEditorNameEntry")->SetValue(_conversation.name);

	findNamedObject<wxCheckBox>(this, "ConvEditorActorsWithinTalkDistance")->SetValue(
		_conversation.actorsMustBeWithinTalkdistance);
	findNamedObject<wxCheckBox>(this, "ConvEditorActorsMustFace")->SetValue(
		_conversation.actorsAlwaysFaceEachOther);

	// Update the max play count
	if (_conversation.maxPlayCount != -1)
	{
		// Max play count is enabled
		findNamedObject<wxSpinCtrl>(this, "ConvEditorRepeatTimes")->Enable(true);
		findNamedObject<wxSpinCtrl>(this, "ConvEditorRepeatTimes")->SetValue(_conversation.maxPlayCount);

		findNamedObject<wxStaticText>(this, "ConvEditorRepeatAdditionalText")->Enable(true);
		findNamedObject<wxCheckBox>(this, "ConvEditorRepeatCheckbox")->SetValue(true);
	}
	else
	{
		// Max play count disabled
		findNamedObject<wxSpinCtrl>(this, "ConvEditorRepeatTimes")->Enable(false);
		findNamedObject<wxSpinCtrl>(this, "ConvEditorRepeatTimes")->SetValue(-1);

		findNamedObject<wxStaticText>(this, "ConvEditorRepeatAdditionalText")->Enable(false);
		findNamedObject<wxCheckBox>(this, "ConvEditorRepeatCheckbox")->SetValue(false);
	}

	// Actors
	for (conversation::Conversation::ActorMap::const_iterator i = _conversation.actors.begin();
		 i != _conversation.actors.end(); ++i)
	{
		wxutil::TreeModel::Row row = _actorStore->AddItem();

		row[_actorColumns.actorNumber] = i->first;
		row[_actorColumns.displayName] = i->second;

		row.SendItemAdded();
	}

	// Commands
	for (conversation::Conversation::CommandMap::const_iterator i = _conversation.commands.begin();
		 i != _conversation.commands.end(); ++i)
	{
		const conversation::ConversationCommand& cmd = *(i->second);

		wxutil::TreeModel::Row row = _commandStore->AddItem();

		row[_commandColumns.cmdNumber] = i->first;
		row[_commandColumns.actorName] = (boost::format(_("Actor %d")) % cmd.actor).str();
		row[_commandColumns.sentence] = removeMarkup(cmd.getSentence());
		row[_commandColumns.wait] = cmd.waitUntilFinished ? _("yes") : _("no");

		row.SendItemAdded();
	}

	_updateInProgress = false;
}

void ConversationEditor::selectCommand(int index)
{
	// Select the actor passed from the command
	wxDataViewItem found = _commandStore->FindInteger(index, _commandColumns.cmdNumber.getColumnIndex());
	_commandView->Select(found);

	// Update sensitivity based on the new selection
	_currentCommand = _commandView->GetSelection();
	updateCmdActionSensitivity(_currentCommand.IsOk());
}

void ConversationEditor::moveSelectedCommand(int delta)
{
	// Get the index of the currently selected command
	wxutil::TreeModel::Row row(_currentCommand, *_commandStore);
	int index = row[_commandColumns.cmdNumber].getInteger();

	int targetIndex = index + delta;

	if (targetIndex <= 0)
	{
		return; // can't move any more upwards
	}

	// Try to look up the command indices in the conversation
	conversation::Conversation::CommandMap::iterator oldCmd = _conversation.commands.find(index);
	conversation::Conversation::CommandMap::iterator newCmd = _conversation.commands.find(targetIndex);

	if (oldCmd != _conversation.commands.end() && newCmd != _conversation.commands.end())
	{
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
	_conversation.name = findNamedObject<wxTextCtrl>(this, "ConvEditorNameEntry")->GetValue();

	_conversation.actorsMustBeWithinTalkdistance = 
		findNamedObject<wxCheckBox>(this, "ConvEditorActorsWithinTalkDistance")->GetValue();
	_conversation.actorsAlwaysFaceEachOther = 
		findNamedObject<wxCheckBox>(this, "ConvEditorActorsMustFace")->GetValue();

	if (findNamedObject<wxCheckBox>(this, "ConvEditorRepeatCheckbox")->GetValue())
	{
		_conversation.maxPlayCount = findNamedObject<wxSpinCtrl>(this, "ConvEditorRepeatTimes")->GetValue();
	}
	else
	{
		_conversation.maxPlayCount = -1;
	}

	// Copy the working copy over the actual object
	_targetConversation = _conversation;
}

void ConversationEditor::onSave(wxCommandEvent& ev)
{
	// First, save to the conversation object
	save();

	// Then close the window
	EndModal(wxID_OK);
}

void ConversationEditor::onCancel(wxCommandEvent& ev)
{
	EndModal(wxID_CANCEL);
}

void ConversationEditor::onActorSelectionChanged(wxDataViewEvent& ev)
{
	if (_updateInProgress) return;

	// Get the selection
	_currentActor = _actorView->GetSelection();

	// Enable the delete buttons if we have a selection
	_delActorButton->Enable(_currentActor.IsOk());
}

void ConversationEditor::updateCmdActionSensitivity(bool hasSelection)
{
	// Enable the edit and delete buttons if we have a selection
	_editCmdButton->Enable(hasSelection);
	_delCmdButton->Enable(hasSelection);

	if (hasSelection)
	{
		// Check if this is the first command in the list, get the ID of the selected item
		wxutil::TreeModel::Row row(_currentCommand, *_commandStore);
		int index = row[_commandColumns.cmdNumber].getInteger();

		bool hasNext = _conversation.commands.find(index+1) != _conversation.commands.end();
		bool hasPrev = index > 1;

		_moveUpCmdButton->Enable(hasPrev);
		_moveDownCmdButton->Enable(hasNext);
	}
	else
	{
		_moveUpCmdButton->Enable(false);
		_moveDownCmdButton->Enable(false);
	}
}

void ConversationEditor::onCommandSelectionChanged(wxDataViewEvent& ev)
{
	if (_updateInProgress) return;

	// Get the selection
	_currentCommand = _commandView->GetSelection();

	updateCmdActionSensitivity(_currentCommand.IsOk());
}

void ConversationEditor::onMaxPlayCountEnabled(wxCommandEvent& ev)
{
	if (_updateInProgress) return;

	if (findNamedObject<wxCheckBox>(this, "ConvEditorRepeatCheckbox")->GetValue())
	{
		// Enabled, write a new value in the spin button
		findNamedObject<wxSpinCtrl>(this, "ConvEditorRepeatTimes")->SetValue(1);
		findNamedObject<wxSpinCtrl>(this, "ConvEditorRepeatTimes")->Enable(true);
		findNamedObject<wxStaticText>(this, "ConvEditorRepeatAdditionalText")->Enable(true);
	}
	else
	{
		// Disabled, write a -1 in the spin button
		findNamedObject<wxSpinCtrl>(this, "ConvEditorRepeatTimes")->SetValue(-1);

		findNamedObject<wxSpinCtrl>(this, "ConvEditorRepeatTimes")->Enable(false);
		findNamedObject<wxStaticText>(this, "ConvEditorRepeatAdditionalText")->Enable(false);
	}
}

void ConversationEditor::onAddActor(wxCommandEvent& ev)
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

void ConversationEditor::onDeleteActor(wxCommandEvent& ev)
{
	// Get the index of the currently selected actor
	wxutil::TreeModel::Row row(_currentActor, *_actorStore);
	int index = row[_actorColumns.actorNumber].getInteger();

	// Add the new actor to the map
	conversation::Conversation::ActorMap::iterator i = _conversation.actors.find(index);

	if (i != _conversation.actors.end())
	{
		// Remove the specified actor
		_conversation.actors.erase(index);
	}
	else
	{
		// Index not found, quit here
		return;
	}

	// Adjust the numbers of all other actors with higher numbers
	while (_conversation.actors.find(index + 1) != _conversation.actors.end()) 
	{
		// Move the actor with the higher index "down" by one number...
		_conversation.actors[index] = _conversation.actors[index + 1];
		// ...and remove it from the old location
		_conversation.actors.erase(index + 1);

		index++;
	}

	// Update the widgets
	updateWidgets();
}

void ConversationEditor::onActorEdited(wxDataViewEvent& ev)
{
	wxutil::TreeModel::Row row(ev.GetItem(), *_actorStore);

	// The iter points to the edited cell now, get the actor number
	int actorNum = row[_actorColumns.actorNumber].getInteger();

	// Update the conversation
	_conversation.actors[actorNum] = static_cast<std::string>(ev.GetValue());

	// Update all widgets
	updateWidgets();
}

void ConversationEditor::onAddCommand(wxCommandEvent& ev)
{
	conversation::Conversation& conv = _conversation; // shortcut

	// Create a new command
	conversation::ConversationCommandPtr command(new conversation::ConversationCommand);

	// Construct a command editor (blocks on construction)
	CommandEditor* editor = new CommandEditor(this, *command, conv);

	if (editor->ShowModal() == wxID_OK)
	{
		// The user hit ok, insert the command, find the first free index
		int index = 1;
		while (conv.commands.find(index) != conv.commands.end())
		{
			index++;
		}

		// Insert the command at the new location
		conv.commands[index] = command;

		updateWidgets();
	}

	editor->Destroy();
}

void ConversationEditor::onEditCommand(wxCommandEvent& ev)
{
	// Get the index of the currently selected command
	wxutil::TreeModel::Row row(_currentCommand, *_commandStore);
	int index = row[_commandColumns.cmdNumber].getInteger();

	// Try to look up the command in the conversation
	conversation::Conversation::CommandMap::iterator i = _conversation.commands.find(index);

	if (i != _conversation.commands.end())
	{
		// Get the command reference
		conversation::ConversationCommandPtr command = i->second;

		// Construct a command editor (blocks on construction)
		CommandEditor* editor = new CommandEditor(this, *command, _conversation);

		if (editor->ShowModal() == wxID_OK)
		{
			updateWidgets();
		}

		editor->Destroy();
	}
}

void ConversationEditor::onMoveUpCommand(wxCommandEvent& ev)
{
	// Pass the call
	moveSelectedCommand(-1);
}

void ConversationEditor::onMoveDownCommand(wxCommandEvent& ev)
{
	// Pass the call
	moveSelectedCommand(+1);
}

void ConversationEditor::onDeleteCommand(wxCommandEvent& ev)
{
	// Get the index of the currently selected command
	wxutil::TreeModel::Row row(_currentCommand, *_commandStore);
	int index = row[_commandColumns.cmdNumber].getInteger();

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

std::string ConversationEditor::removeMarkup(const std::string& input)
{
	boost::regex expr("(<[A-Za-z]+>)|(</[A-Za-z]+>)");
	return boost::regex_replace(input, expr, "");
}

} // namespace ui
