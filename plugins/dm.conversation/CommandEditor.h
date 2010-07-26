#ifndef CONVERSATION_COMMAND_EDITOR_H_
#define CONVERSATION_COMMAND_EDITOR_H_

#include <gtk/gtkliststore.h>
#include "gtkutil/window/BlockingTransientWindow.h"

#include "Conversation.h"
#include "ConversationCommand.h"
#include "CommandArgumentItem.h"

namespace ui {

class CommandEditor :
	public gtkutil::BlockingTransientWindow
{
public:
	// Whether the user clicked on cancel or OK
	enum Result {
		RESULT_CANCEL,
		RESULT_OK,
		NUM_RESULTS
	};

private:
	// The conversation (read-only)
	const conversation::Conversation& _conversation;

	// The command we're editing (working copy)
	conversation::ConversationCommand _command;

	// The actual command we're saving to on "OK"
	conversation::ConversationCommand& _targetCommand;

	Result _result;

	// All available actors
	GtkListStore* _actorStore;
	GtkWidget* _actorDropDown;
	
	// All available commands
	GtkListStore* _commandStore;
	GtkWidget* _commandDropDown;

	GtkWidget* _waitUntilFinished;

	GtkWidget* _argAlignment;
	GtkWidget* _argTable;

	// Widget pointer to keep track of the widget in the _argAlignment;
	GtkWidget* _argumentWidget;

	// The tooltips group to display the help text
	GtkTooltips* _tooltips;

	typedef std::vector<CommandArgumentItemPtr> ArgumentItemList;
	ArgumentItemList _argumentItems;

public:
	// Pass the parent window, the command and the conversation to edit
	CommandEditor(const Glib::RefPtr<Gtk::Window>& parent, conversation::ConversationCommand& command, conversation::Conversation conv);

	// Determine which action the user did take to close the dialog
	Result getResult();

private:
	void populateWindow();
	void updateWidgets();

	void save();

	void commandTypeChanged();

	void createArgumentWidgets(int commandTypeID);

	void upateWaitUntilFinished(int commandTypeID);

	GtkWidget* createButtonPanel();

	static void onSave(GtkWidget* button, CommandEditor* self);
	static void onCancel(GtkWidget* button, CommandEditor* self);

	static void onCommandTypeChange(GtkWidget* combobox, CommandEditor* self);
};

} // namespace ui

#endif /* CONVERSATION_COMMAND_EDITOR_H_ */
