#ifndef CONVERSATION_EDITOR_H_
#define CONVERSATION_EDITOR_H_

#include "gtkutil/window/BlockingTransientWindow.h"
#include <map>

#include "Conversation.h"

typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GtkCellRendererText GtkCellRendererText;

namespace ui {

class ConversationEditor :
	public gtkutil::BlockingTransientWindow
{
	// The overall dialog vbox (used to quickly disable the whole dialog)
	GtkWidget* _dialogVBox;

	GtkListStore* _actorStore;
	GtkListStore* _commandStore;

	std::map<int, GtkWidget*> _widgets;
	
	// The conversation we're editing
	conversation::Conversation& _conversation;

public:
	ConversationEditor(GtkWindow* parent, conversation::Conversation& conversation);

private:
	void save();

	void populateWindow();

	// Fills the conversation values into the widgets
	void populateWidgets();

	GtkWidget* createPropertyPane();
	GtkWidget* createButtonPanel();
	GtkWidget* createActorPanel();
	GtkWidget* createCommandPanel();

	static void onSave(GtkWidget* button, ConversationEditor* self);
	static void onCancel(GtkWidget* button, ConversationEditor* self);
	static void onActorSelectionChanged(GtkTreeSelection* sel, ConversationEditor* self);
	static void onCommandSelectionChanged(GtkTreeSelection* sel, ConversationEditor* self);

	static void onAddActor(GtkWidget* w, ConversationEditor* self);
	static void onDeleteActor(GtkWidget* w, ConversationEditor* self);
	static void onActorEdited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, ConversationEditor* self);

}; // class ConversationEditor

} // namespace ui

#endif /* CONVERSATION_EDITOR_H_ */
