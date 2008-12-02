#ifndef CONVERSATION_EDITOR_H_
#define CONVERSATION_EDITOR_H_

#include <gtk/gtktreemodel.h>
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
	GtkListStore* _actorStore;
	GtkListStore* _commandStore;

	GtkTreeIter _currentActor;
	GtkTreeIter _currentCommand;

	std::map<int, GtkWidget*> _widgets;
	
	// The conversation we're editing (the working copy)
	conversation::Conversation _conversation;

	// The actual conversation, where the changes will be saved to on "OK"
	conversation::Conversation& _targetConversation;

public:
	ConversationEditor(GtkWindow* parent, conversation::Conversation& conversation);

private:
	void save();

	void populateWindow();

	// Fills the conversation values into the widgets
	void updateWidgets();

	void updateCmdActionSensitivity(bool hasSelection);

	GtkWidget* createPropertyPane();
	GtkWidget* createButtonPanel();
	GtkWidget* createActorPanel();
	GtkWidget* createCommandPanel();

	// Move the currently selected command about the given delta 
	// (-1 is one upwards, +1 is one position downards)
	void moveSelectedCommand(int delta);

	// Highlight the command with the given index
	void selectCommand(int index);

	static void onSave(GtkWidget* button, ConversationEditor* self);
	static void onCancel(GtkWidget* button, ConversationEditor* self);

	static void onActorSelectionChanged(GtkTreeSelection* sel, ConversationEditor* self);
	static void onCommandSelectionChanged(GtkTreeSelection* sel, ConversationEditor* self);

	static void onAddActor(GtkWidget* w, ConversationEditor* self);
	static void onDeleteActor(GtkWidget* w, ConversationEditor* self);
	static void onActorEdited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, ConversationEditor* self);

	static void onAddCommand(GtkWidget* w, ConversationEditor* self);
	static void onEditCommand(GtkWidget* w, ConversationEditor* self);
	static void onMoveUpCommand(GtkWidget* w, ConversationEditor* self);
	static void onMoveDownCommand(GtkWidget* w, ConversationEditor* self);
	static void onDeleteCommand(GtkWidget* w, ConversationEditor* self);

}; // class ConversationEditor

} // namespace ui

#endif /* CONVERSATION_EDITOR_H_ */
