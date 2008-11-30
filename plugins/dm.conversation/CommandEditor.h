#ifndef CONVERSATION_COMMAND_EDITOR_H_
#define CONVERSATION_COMMAND_EDITOR_H_

#include "gtkutil/window/BlockingTransientWindow.h"

#include "ConversationCommand.h"

namespace ui {

class CommandEditor :
	public gtkutil::BlockingTransientWindow
{
	// The overall dialog vbox (used to quickly disable the whole dialog)
	GtkWidget* _dialogVBox;

	// The command we're editing
	conversation::ConversationCommand& _command;

public:
	// Pass the parent window and the command to edit
	CommandEditor(GtkWindow* parent, conversation::ConversationCommand& command);
};

} // namespace ui

#endif /* CONVERSATION_COMMAND_EDITOR_H_ */
