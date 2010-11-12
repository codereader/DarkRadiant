#ifndef CONVERSATION_COMMAND_EDITOR_H_
#define CONVERSATION_COMMAND_EDITOR_H_

#include <gtkmm/liststore.h>
#include "gtkutil/window/BlockingTransientWindow.h"

#include "Conversation.h"
#include "ConversationCommand.h"
#include "ConversationCommandLibrary.h"
#include "CommandArgumentItem.h"

namespace Gtk
{
	class ComboBox;
	class CheckButton;
	class Alignment;
	class Table;
	class Widget;
}

namespace ui
{

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
	ActorColumns _actorColumns;
	Glib::RefPtr<Gtk::ListStore> _actorStore;
	Gtk::ComboBox* _actorDropDown;

	// All available commands
	conversation::CommandColumns _commandColumns;
	Glib::RefPtr<Gtk::ListStore> _commandStore;
	Gtk::ComboBox* _commandDropDown;

	Gtk::CheckButton* _waitUntilFinished;

	Gtk::Alignment* _argAlignment;
	Gtk::Table* _argTable;

	// Widget pointer to keep track of the widget in the _argAlignment;
	Gtk::Widget* _argumentWidget;

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

	Gtk::Widget& createButtonPanel();

	void onSave();
	void onCancel();

	void onCommandTypeChange();
};

} // namespace ui

#endif /* CONVERSATION_COMMAND_EDITOR_H_ */
