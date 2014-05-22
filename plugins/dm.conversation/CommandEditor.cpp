#include "CommandEditor.h"

#include "i18n.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"
#include "string/convert.h"

#include <boost/format.hpp>
#include "itextstream.h"

#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/button.h>

#include "ConversationCommandLibrary.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Edit Command");
}

CommandEditor::CommandEditor(wxWindow* parent, conversation::ConversationCommand& command, conversation::Conversation conv) :
	DialogBase(_(WINDOW_TITLE), parent),
	_conversation(conv),
	_command(command), // copy the conversation command to a local object
	_targetCommand(command),
	_argTable(NULL),
	_argumentWidget(NULL)
{
	// Create all widgets
	populateWindow();

	// Fill the actor store
	int dropDownIndex = 0;
	for (conversation::Conversation::ActorMap::const_iterator i = _conversation.actors.begin();
		 i != _conversation.actors.end(); ++i)
	{
		std::string actorStr = (boost::format(_("Actor %d (%s)")) % i->first % i->second).str();

		// Store the actor ID into a client data object and pass it along
		findNamedObject<wxChoice>(this, "ConvCmdEditorActorChoice")->Append(actorStr, 
			new wxStringClientData(string::to_string(i->first)));
	}

	// Let the command library fill the command dropdown
	conversation::ConversationCommandLibrary::Instance().populateChoice(
		findNamedObject<wxChoice>(this, "ConvCmdEditorCommandChoice"));

	// Fill the values
	updateWidgets();
}

void CommandEditor::selectItemByStoredId(wxChoice* choice, int id)
{
	choice->SetSelection(wxNOT_FOUND);
	
	for (int i = 0; i < choice->GetCount(); ++i)
	{
		wxStringClientData* cmdIdStr = static_cast<wxStringClientData*>(choice->GetClientObject(i));
		int cmdId = string::convert<int>(cmdIdStr->GetData().ToStdString(), wxNOT_FOUND);

		if (cmdId == id)
		{
			choice->SetSelection(i);
			return;
		}
	}
}

void CommandEditor::updateWidgets()
{
	// Select the actor passed from the command
	selectItemByStoredId(findNamedObject<wxChoice>(this, "ConvCmdEditorActorChoice"), _command.actor);

	// Select the command type
	selectItemByStoredId(findNamedObject<wxChoice>(this, "ConvCmdEditorActorChoice"), _command.type);

	// Populate the correct command argument widgets
	createArgumentWidgets(_command.type);

	// wxTODO Pre-fill the values
	for (conversation::ConversationCommand::ArgumentMap::const_iterator i = _command.arguments.begin();
		i != _command.arguments.end(); ++i)
	{
		int argIndex = i->first;

		if (argIndex > static_cast<int>(_argumentItems.size()) || argIndex < 0)
		{
			// Invalid command argument index
			rError() << "Invalid command argument index " << argIndex << std::endl;
			continue;
		}

		// Load the value into the argument item
		_argumentItems[argIndex - 1]->setValueFromString(i->second);
	}

	// Update the "wait until finished" flag
	findNamedObject<wxCheckBox>(this, "ConvCmdEditorWaitUntilFinished")->SetValue(_command.waitUntilFinished);

	// Update the sensitivity of the correct flag
	upateWaitUntilFinished(_command.type);
}

void CommandEditor::save()
{
	// Get the active actor item
	Gtk::TreeModel::iterator actor = _actorDropDown->get_active();

	if (actor)
	{
		_command.actor = (*actor)[_actorColumns.actorNumber];
	}

	// Get the active command type selection
	Gtk::TreeModel::iterator cmd = _commandDropDown->get_active();

	if (cmd)
	{
		_command.type = (*cmd)[_commandColumns.cmdNumber];
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
	try
	{
		const conversation::ConversationCommandInfo& cmdInfo =
			conversation::ConversationCommandLibrary::Instance().findCommandInfo(_command.type);

		if (cmdInfo.waitUntilFinishedAllowed)
		{
			// Load the value
			_command.waitUntilFinished = _waitUntilFinished->get_active();
		}
		else {
			// Command doesn't support "wait until finished", set to default == true
			_command.waitUntilFinished = true;
		}
	}
	catch (std::runtime_error&)
	{
		rError() << "Cannot find conversation command info for index " << _command.type << std::endl;
	}

	// Copy the command over the target object
	_targetCommand = _command;
}

void CommandEditor::populateWindow()
{
	loadNamedPanel(this, "ConvCmdEditorMainPanel");

	makeLabelBold(this, "ConvCmdEditorActorLabel");
	makeLabelBold(this, "ConvCmdEditorCommandLabel");
	makeLabelBold(this, "ConvCmdEditorCmdArgLabel");
	makeLabelBold(this, "ConvCmdEditorPropertiesLabel");

	wxChoice* cmdDropDown = findNamedObject<wxChoice>(this, "ConvCmdEditorCommandChoice");
	cmdDropDown->Connect(wxEVT_CHOICE, wxCommandEventHandler(CommandEditor::onCommandTypeChange), NULL, this);

	// Wire up button events
	findNamedObject<wxButton>(this, "ConvCmdEditorCancelButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(CommandEditor::onCancel), NULL, this);
	findNamedObject<wxButton>(this, "ConvCmdEditorOkButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(CommandEditor::onSave), NULL, this);
}

void CommandEditor::commandTypeChanged()
{
	int newCommandTypeID = -1;

	wxChoice* cmdDropDown = findNamedObject<wxChoice>(this, "ConvCmdEditorCommandChoice");
	int selectedItem = cmdDropDown->GetSelection();

	wxStringClientData* cmdIdStr = static_cast<wxStringClientData*>(cmdDropDown->GetClientObject(selectedItem));
	newCommandTypeID = string::convert<int>(cmdIdStr->GetData().ToStdString(), -1);

	// Create the argument widgets for this new command type
	createArgumentWidgets(newCommandTypeID);

	// Update the sensitivity of the correct flag
	upateWaitUntilFinished(newCommandTypeID);
}

void CommandEditor::upateWaitUntilFinished(int commandTypeID)
{
	// Update the sensitivity of the correct flag
	try
	{
		const conversation::ConversationCommandInfo& cmdInfo =
			conversation::ConversationCommandLibrary::Instance().findCommandInfo(commandTypeID);

		findNamedObject<wxCheckBox>(this, "ConvCmdEditorWaitUntilFinished")->Enable(cmdInfo.waitUntilFinishedAllowed);
	}
	catch (std::runtime_error&)
	{
		rError() << "Cannot find conversation command info for index " << commandTypeID << std::endl;
	}
}

void CommandEditor::createArgumentWidgets(int commandTypeID)
{
	try
	{
		const conversation::ConversationCommandInfo& cmdInfo =
			conversation::ConversationCommandLibrary::Instance().findCommandInfo(commandTypeID);

		// Remove all possible previous items from the list
		_argumentItems.clear();

		// Clear the panel and add a new table
		wxPanel* argPanel = findNamedObject<wxPanel>(this, "ConvCmdEditorArgPanel");
		argPanel->DestroyChildren();

		// Create the table
		argPanel->SetSizer(new wxFlexGridSizer(static_cast<int>(cmdInfo.arguments.size()), 3, 6, 12));

		if (cmdInfo.arguments.empty())
		{
			wxStaticText* noneText = new wxStaticText(argPanel, wxID_ANY, _("None"));
			noneText->SetFont(noneText->GetFont().Italic());
			argPanel->GetSizer()->Add(noneText);
			return;
		}

		// Setup the table with default spacings
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
				item = CommandArgumentItemPtr(new BooleanArgument(argInfo));
				break;
			case conversation::ArgumentInfo::ARGTYPE_INT:
			case conversation::ArgumentInfo::ARGTYPE_FLOAT:
			case conversation::ArgumentInfo::ARGTYPE_STRING:
				// Create a new string argument item
				item = CommandArgumentItemPtr(new StringArgument(argInfo));
				break;
			case conversation::ArgumentInfo::ARGTYPE_VECTOR:
			case conversation::ArgumentInfo::ARGTYPE_SOUNDSHADER:
				// Create a new string argument item
				item = CommandArgumentItemPtr(new StringArgument(argInfo));
				break;
			case conversation::ArgumentInfo::ARGTYPE_ACTOR:
				// Create a new actor argument item
				item = CommandArgumentItemPtr(new ActorArgument(argInfo, _actorStore, _actorColumns));
				break;
			case conversation::ArgumentInfo::ARGTYPE_ENTITY:
				// Create a new string argument item
				item = CommandArgumentItemPtr(new StringArgument(argInfo));
				break;
			default:
				rError() << "Unknown command argument type: " << argInfo.type << std::endl;
				break;
			};

			if (item != NULL)
			{
				_argumentItems.push_back(item);

				if (argInfo.type != conversation::ArgumentInfo::ARGTYPE_BOOL)
				{
					// The label
					_argTable->attach(
						item->getLabelWidget(),
						0, 1, index-1, index, // index starts with 1, hence the -1
						Gtk::FILL, Gtk::AttachOptions(0), 0, 0
					);

					// The edit widgets
					_argTable->attach(
						item->getEditWidget(),
						1, 2, index-1, index // index starts with 1, hence the -1
					);
				}
				else
				{
					// This is a checkbutton - should be spanned over two columns
					_argTable->attach(
						item->getEditWidget(),
						0, 2, index-1, index, // index starts with 1, hence the -1
						Gtk::FILL, Gtk::AttachOptions(0), 0, 0
					);
				}

				// The help widgets
				_argTable->attach(
					item->getHelpWidget(),
					2, 3, index-1, index, // index starts with 1, hence the -1
					Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0
				);
			}
		}
	}
	catch (std::runtime_error&)
	{
		rError() << "Cannot find conversation command info for index " << commandTypeID << std::endl;
	}
}

void CommandEditor::onSave(wxCommandEvent& ev)
{
	save();
	EndModal(wxID_OK);
}

void CommandEditor::onCancel(wxCommandEvent& ev)
{
	// Just close the window without writing the values
	EndModal(wxID_CANCEL);
}

void CommandEditor::onCommandTypeChange(wxCommandEvent& ev)
{
	commandTypeChanged();
}

} // namespace ui
