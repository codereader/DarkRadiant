#ifndef CONVERSATION_EDITOR_H_
#define CONVERSATION_EDITOR_H_

#include "gtkutil/window/BlockingTransientWindow.h"

namespace ui {

class ConversationEditor :
	public gtkutil::BlockingTransientWindow
{
	// The overall dialog vbox (used to quickly disable the whole dialog)
	GtkWidget* _dialogVBox;

	// Members: TODO
	
public:
	ConversationEditor();
	

}; // class ConversationEditor

} // namespace ui

#endif /* CONVERSATION_EDITOR_H_ */
