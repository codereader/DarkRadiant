#ifndef CONVERSATION_DIALOG_H_
#define CONVERSATION_DIALOG_H_

#include <map>

#include "ientity.h"
#include "iradiant.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/BlockingTransientWindow.h"

#include "ConversationEntity.h"

// Forward decl.
typedef struct _GtkNotebook GtkNotebook;
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeSelection GtkTreeSelection;

namespace ui {

class ConversationDialog;
typedef boost::shared_ptr<ConversationDialog> ConversationDialogPtr;

/**
 * greebo: The conversation dialog is a modal top-level window providing
 * views and controls to facilitate the setup of inter-AI conversations.
 */
class ConversationDialog :
	public gtkutil::BlockingTransientWindow
{
	// The overall dialog vbox (used to quickly disable the whole dialog)
	GtkWidget* _dialogVBox;

	// List of conversation_info entities
	GtkListStore* _convEntityList;

	// Map of ConversationEntity objects, indexed by the name of the world entity
	conversation::ConversationEntityMap _entities;

	// Table of dialog subwidgets
	std::map<int, GtkWidget*> _widgets;

	// The close button to toggle the view
	GtkWidget* _closeButton;
	
	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;
	
public:
	ConversationDialog();
	
	// Command target to toggle the dialog
	static void showDialog();

private:
	virtual void _preHide();
	virtual void _preShow();
	
	// greebo: Saves the current working set to the entity
	void save();

	void populateWidgets();

	// WIDGET POPULATION
	void populateWindow(); 			// Main window
	GtkWidget* createEntitiesPanel();
	GtkWidget* createButtons(); 	// Dialog buttons
	
	// Button callbacks
	static void onSave(GtkWidget* button, ConversationDialog* self);
	static void onClose(GtkWidget* button, ConversationDialog* self);
	static void onEntitySelectionChanged(GtkTreeSelection*, ConversationDialog*);
	static void onAddEntity(GtkWidget*, ConversationDialog*);
	static void onDeleteEntity(GtkWidget*, ConversationDialog*);

	// The keypress handler for catching the keys in the treeview
	static gboolean onWindowKeyPress(GtkWidget* dialog, GdkEventKey* event, ConversationDialog* self);

}; // class ConversationDialog

} // namespace ui

#endif /*CONVERSATION_DIALOG_H_*/
