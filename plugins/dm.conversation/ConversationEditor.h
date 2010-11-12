#ifndef CONVERSATION_EDITOR_H_
#define CONVERSATION_EDITOR_H_

#include <gtkmm/liststore.h>
#include "gtkutil/window/BlockingTransientWindow.h"
#include <map>

#include "Conversation.h"

namespace Gtk
{
	class Entry;
	class CheckButton;
	class SpinButton;
	class Button;
	class TreeView;
	class HBox;
}

namespace ui
{

class ConversationEditor :
	public gtkutil::BlockingTransientWindow
{
private:
	struct ActorListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ActorListColumns() { add(actorNumber); add(displayName); }

		Gtk::TreeModelColumn<int> actorNumber;				// actor number
		Gtk::TreeModelColumn<Glib::ustring> displayName;	// display name
	};

	ActorListColumns _actorColumns;
	Glib::RefPtr<Gtk::ListStore> _actorStore;
	Gtk::TreeView* _actorView;

	struct CommandListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		CommandListColumns()
		{
			add(cmdNumber);
			add(actorName);
			add(sentence);
			add(wait);
		}

		Gtk::TreeModelColumn<int> cmdNumber;			// cmd number
		Gtk::TreeModelColumn<Glib::ustring> actorName;	// actor name
		Gtk::TreeModelColumn<Glib::ustring> sentence;	// sentence
		Gtk::TreeModelColumn<Glib::ustring> wait;		// wait yes/no
	};

	CommandListColumns _commandColumns;
	Glib::RefPtr<Gtk::ListStore> _commandStore;
	Gtk::TreeView* _commandView;

	Gtk::TreeModel::iterator _currentActor;
	Gtk::TreeModel::iterator _currentCommand;

	Gtk::Entry* _convNameEntry;
	Gtk::CheckButton* _convActorsWithinTalkDistance;
	Gtk::CheckButton* _convActorsAlwaysFace;
	Gtk::CheckButton* _convMaxPlayCountEnable;
	Gtk::HBox* _maxPlayCountHBox;
	Gtk::SpinButton* _maxPlayCount;

	Gtk::Button* _addActorButton;
	Gtk::Button* _delActorButton;

	Gtk::Button* _addCmdButton;
	Gtk::Button* _delCmdButton;
	Gtk::Button* _editCmdButton;
	Gtk::Button* _moveUpCmdButton;
	Gtk::Button* _moveDownCmdButton;

	// The conversation we're editing (the working copy)
	conversation::Conversation _conversation;

	// The actual conversation, where the changes will be saved to on "OK"
	conversation::Conversation& _targetConversation;

	// Mutex to avoid callback loops
	bool _updateInProgress;

public:
	ConversationEditor(const Glib::RefPtr<Gtk::Window>& parent, conversation::Conversation& conversation);

private:
	void save();

	void populateWindow();

	// Fills the conversation values into the widgets
	void updateWidgets();

	void updateCmdActionSensitivity(bool hasSelection);

	Gtk::Widget& createPropertyPane();
	Gtk::Widget& createButtonPanel();
	Gtk::Widget& createActorPanel();
	Gtk::Widget& createCommandPanel();

	// Move the currently selected command about the given delta
	// (-1 is one upwards, +1 is one position downards)
	void moveSelectedCommand(int delta);

	// Highlight the command with the given index
	void selectCommand(int index);

	void onSave();
	void onCancel();

	void onMaxPlayCountEnabled();

	void onActorSelectionChanged();
	void onCommandSelectionChanged();

	void onAddActor();
	void onDeleteActor();
	void onActorEdited(const Glib::ustring& path, const Glib::ustring& new_text);

	void onAddCommand();
	void onEditCommand();
	void onMoveUpCommand();
	void onMoveDownCommand();
	void onDeleteCommand();

}; // class ConversationEditor

} // namespace ui

#endif /* CONVERSATION_EDITOR_H_ */
