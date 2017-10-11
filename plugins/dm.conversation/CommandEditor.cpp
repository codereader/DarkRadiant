#include "CommandEditor.h"

#include "i18n.h"
#include "string/string.h"
#include "string/convert.h"

#include <fmt/format.h>
#include "itextstream.h"

#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/button.h>

#include "wxutil/ChoiceHelper.h"
#include "ConversationCommandLibrary.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Edit Command");
}

CommandEditor::CommandEditor(wxWindow* parent, conversation::ConversationCommand& command, const conversation::Conversation& conv) :
	DialogBase(_(WINDOW_TITLE), parent),
	_conversation(conv),
	_command(command), // copy the conversation command to a local object
	_targetCommand(command)
{
	// Create all widgets
	populateWindow();

	// Fill the actor dropdown
	for (conversation::Conversation::ActorMap::const_iterator i = _conversation.actors.begin();
		 i != _conversation.actors.end(); ++i)
	{
		std::string actorStr = fmt::format(_("Actor {0:d} ({1})"), i->first, i->second);

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

conversation::ConversationCommand& CommandEditor::getCommand()
{
	return _command;
}

const conversation::Conversation& CommandEditor::getConversation()
{
	return _conversation;
}

void CommandEditor::updateWidgets()
{
	// Select the actor passed from the command
	wxutil::ChoiceHelper::SelectItemByStoredId(
		findNamedObject<wxChoice>(this, "ConvCmdEditorActorChoice"), _command.actor);

	// Select the command type
	wxutil::ChoiceHelper::SelectItemByStoredId(
		findNamedObject<wxChoice>(this, "ConvCmdEditorCommandChoice"), _command.type);

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
			rError() << "Invalid command argument index " << argIndex << std::endl;
			continue;
		}

		// Load the value into the argument item
		_argumentItems[argIndex - 1]->setValueFromString(i->second);
	}

	// Update the "wait until finished" flag
	findNamedObject<wxCheckBox>(this, "ConvCmdEditorWaitUntilFinished")->SetValue(_command.waitUntilFinished);

	// Update the sensitivity of the correct flag
	updateWaitUntilFinished(_command.type);
}

void CommandEditor::save()
{
	_command.actor = wxutil::ChoiceHelper::GetSelectionId(
		findNamedObject<wxChoice>(this, "ConvCmdEditorActorChoice"));

	_command.type = wxutil::ChoiceHelper::GetSelectionId(
		findNamedObject<wxChoice>(this, "ConvCmdEditorCommandChoice"));

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
			_command.waitUntilFinished = 
				findNamedObject<wxCheckBox>(this, "ConvCmdEditorWaitUntilFinished")->GetValue();
		}
		else
		{
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
	updateWaitUntilFinished(newCommandTypeID);
}

void CommandEditor::updateWaitUntilFinished(int commandTypeID)
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
		wxFlexGridSizer* table = new wxFlexGridSizer(static_cast<int>(cmdInfo.arguments.size()), 3, 6, 12);
		table->AddGrowableCol(1);

		argPanel->SetSizer(table);

		if (cmdInfo.arguments.empty())
		{
			wxStaticText* noneText = new wxStaticText(argPanel, wxID_ANY, _("None"));
			noneText->SetFont(noneText->GetFont().Italic());
			argPanel->GetSizer()->Add(noneText, 0, wxLEFT, 6);
			return;
		}

		// Setup the table with default spacings
		for (const conversation::ArgumentInfo& argInfo : cmdInfo.arguments)
		{
			CommandArgumentItemPtr item = createCommandArgumentItem(argInfo, argPanel);

			if (item)
			{
				_argumentItems.push_back(item);

				// The label
				table->Add(item->getLabelWidget(), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

				// The edit widgets
				table->Add(item->getEditWidget(), 1, wxEXPAND, wxALIGN_CENTER_VERTICAL);
				
				// The help widgets
				table->Add(item->getHelpWidget(), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxRIGHT, 6);
			}
		}
	}
	catch (std::runtime_error&)
	{
		rError() << "Cannot find conversation command info for index " << commandTypeID << std::endl;
	}

	wxPanel* mainPanel = findNamedObject<wxPanel>(this, "ConvCmdEditorMainPanel");
	mainPanel->Fit();
	mainPanel->Layout();
	Fit();
}

CommandArgumentItemPtr CommandEditor::createCommandArgumentItem(const conversation::ArgumentInfo& argInfo, wxWindow* parent)
{
	// Unfortunately we don't have a type declaration for animation in the .def file (didn't think about that back then)
	// so let's detect the "Anim" title of the argument and construct an animation picker in this case
	if (argInfo.title == "Anim")
	{
		return std::make_shared<AnimationArgument>(*this, parent, argInfo);
	}

	switch (argInfo.type)
	{
	case conversation::ArgumentInfo::ARGTYPE_BOOL:
		// Create a new bool argument item
		return std::make_shared<BooleanArgument>(*this, parent, argInfo);
	case conversation::ArgumentInfo::ARGTYPE_INT:
	case conversation::ArgumentInfo::ARGTYPE_FLOAT:
	case conversation::ArgumentInfo::ARGTYPE_STRING:
		// Create a new string argument item
		return std::make_shared<StringArgument>(*this, parent, argInfo);
	case conversation::ArgumentInfo::ARGTYPE_VECTOR:
		// Create a new string argument item
		return std::make_shared<StringArgument>(*this, parent, argInfo);
	case conversation::ArgumentInfo::ARGTYPE_SOUNDSHADER:
		// Create a new sound shader argument item
		return std::make_shared<SoundShaderArgument>(*this, parent, argInfo);
	case conversation::ArgumentInfo::ARGTYPE_ACTOR:
		// Create a new actor argument item
		return std::make_shared<ActorArgument>(*this, parent, argInfo, _conversation.actors);
	case conversation::ArgumentInfo::ARGTYPE_ENTITY:
		// Create a new string argument item
		return std::make_shared<StringArgument>(*this, parent, argInfo);
	default:
		rError() << "Unknown command argument type: " << argInfo.type << std::endl;
		break;
	};

	return CommandArgumentItemPtr();
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
