#pragma once

#include "wxutil/TreeModel.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"

#include "Conversation.h"
#include "ConversationCommand.h"
#include "ConversationCommandLibrary.h"
#include "CommandArgumentItem.h"

namespace ui
{

class CommandEditor :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
private:
	// The conversation (read-only)
	const conversation::Conversation& _conversation;

	// The command we're editing (working copy)
	conversation::ConversationCommand _command;

	// The actual command we're saving to on "OK"
	conversation::ConversationCommand& _targetCommand;

	typedef std::vector<CommandArgumentItemPtr> ArgumentItemList;
	ArgumentItemList _argumentItems;

public:
	// Pass the parent window, the command and the conversation to edit
	CommandEditor(wxWindow* parent, conversation::ConversationCommand& command, const conversation::Conversation& conv);

private:
	void populateWindow();
	void updateWidgets();

	void save();

	void commandTypeChanged();

	void createArgumentWidgets(int commandTypeID);

	void upateWaitUntilFinished(int commandTypeID);

	void onSave(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);

	void onCommandTypeChange(wxCommandEvent& ev);
};

} // namespace ui
