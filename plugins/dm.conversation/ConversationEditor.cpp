#include "ConversationEditor.h"

#include "iradiant.h"

namespace ui {

namespace {
	const std::string WINDOW_TITLE = "Edit Conversation";
}

ConversationEditor::ConversationEditor() :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow())
{}

} // namespace ui
