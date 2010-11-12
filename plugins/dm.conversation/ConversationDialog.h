#ifndef CONVERSATION_DIALOG_H_
#define CONVERSATION_DIALOG_H_

#include <map>

#include "ieventmanager.h"
#include "ientity.h"
#include "icommandsystem.h"
#include "iradiant.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/BlockingTransientWindow.h"

#include "ConversationEntity.h"
#include <gtkmm/liststore.h>

namespace Gtk
{
	class VBox;
	class Button;
	class TreeView;
}

namespace ui
{

class ConversationDialog;
typedef boost::shared_ptr<ConversationDialog> ConversationDialogPtr;

/**
 * greebo: The conversation dialog is a modal top-level window providing
 * views and controls to facilitate the setup of inter-AI conversations.
 */
class ConversationDialog :
	public gtkutil::BlockingTransientWindow
{
private:
	// The overall dialog vbox (used to quickly disable the whole dialog)
	Gtk::VBox* _dialogVBox;

	// List of conversation_info entities
	conversation::ConvEntityColumns _convEntityColumns;
	Glib::RefPtr<Gtk::ListStore> _convEntityList;
	Gtk::TreeView* _entityView;

	// List of conversations on the selected entity
	conversation::ConversationColumns _convColumns;
	Glib::RefPtr<Gtk::ListStore> _convList;
	Gtk::TreeView* _convView;

	// Map of ConversationEntity objects, indexed by the name of the world entity
	conversation::ConversationEntityMap _entities;

	// Iterators for current entity and current objective
	conversation::ConversationEntityMap::iterator _curEntity;
	Gtk::TreeModel::iterator _currentConversation;

	// The close button to toggle the view
	Gtk::Button* _closeButton;
	Gtk::Button* _deleteEntityButton;
	Gtk::VBox* _convButtonPanel;
	Gtk::Button* _editConvButton;
	Gtk::Button* _delConvButton;
	Gtk::Button* _clearConvButton;

	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;

public:
	ConversationDialog();

	// Command target to toggle the dialog
	static void showDialog(const cmd::ArgumentList& args);

private:
	virtual void _preHide();
	virtual void _preShow();

	// greebo: Saves the current working set to the entity
	void save();

	// Clears out all stored data
	void clear();

	void populateWidgets();

	// Re-loads the conversation from the selected entity
	void refreshConversationList();

	// WIDGET POPULATION
	void populateWindow(); 			// Main window
	Gtk::Widget& createEntitiesPanel();
	Gtk::Widget& createConversationsPanel();
	Gtk::Widget& createButtons(); 	// Dialog buttons

	// Button callbacks
	void onSave();
	void onClose();
	void onEntitySelectionChanged();
	void onAddEntity();
	void onDeleteEntity();

	void onConversationSelectionChanged();
	void onAddConversation();
	void onEditConversation();
	void onDeleteConversation();
	void onClearConversations();

	// The keypress handler for catching the keys in the treeview
	bool onWindowKeyPress(GdkEventKey* ev);

}; // class ConversationDialog

} // namespace ui

#endif /*CONVERSATION_DIALOG_H_*/
