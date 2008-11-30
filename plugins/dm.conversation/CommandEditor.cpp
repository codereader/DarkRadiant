#include "CommandEditor.h"

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Edit Command";
	}

CommandEditor::CommandEditor(GtkWindow* parent, conversation::ConversationCommand& command) :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, parent),
	_command(command)
{}

} // namespace ui
