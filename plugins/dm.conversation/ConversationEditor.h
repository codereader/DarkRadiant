#pragma once

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/TreeView.h"
#include <map>

#include "Conversation.h"

class wxButton;

namespace ui
{

class ConversationEditor :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
private:
	struct ActorListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ActorListColumns() :
			actorNumber(add(wxutil::TreeModel::Column::Integer)),
			displayName(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column actorNumber;	// actor number
		wxutil::TreeModel::Column displayName;	// display name
	};

	ActorListColumns _actorColumns;
	wxutil::TreeModel::Ptr _actorStore;
	wxutil::TreeView* _actorView;

	struct CommandListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		CommandListColumns() :
			cmdNumber(add(wxutil::TreeModel::Column::Integer)),
			actorName(add(wxutil::TreeModel::Column::String)),
			sentence(add(wxutil::TreeModel::Column::String)),
			wait(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column cmdNumber;	// cmd number
		wxutil::TreeModel::Column actorName;	// actor name
		wxutil::TreeModel::Column sentence;		// sentence
		wxutil::TreeModel::Column wait;			// wait yes/no
	};

	CommandListColumns _commandColumns;
	wxutil::TreeModel::Ptr _commandStore;
	wxutil::TreeView* _commandView;

	wxDataViewItem _currentActor;
	wxDataViewItem _currentCommand;

	wxButton* _addActorButton;
	wxButton* _delActorButton;
	wxButton* _validateActorsButton;

	wxButton* _addCmdButton;
	wxButton* _delCmdButton;
	wxButton* _editCmdButton;
	wxButton* _moveUpCmdButton;
	wxButton* _moveDownCmdButton;

	// The conversation we're editing (the working copy)
	conversation::Conversation _conversation;

	// The actual conversation, where the changes will be saved to on "OK"
	conversation::Conversation& _targetConversation;

	// Mutex to avoid callback loops
	bool _updateInProgress;

public:
	ConversationEditor(wxWindow* parent, conversation::Conversation& conversation);

private:
	void save();

	void populateWindow();

	// Fills the conversation values into the widgets
	void updateWidgets();
    void updateCommandList();
	void updateCmdActionSensitivity(bool hasSelection);

	// Move the currently selected command about the given delta
	// (-1 is one upwards, +1 is one position downards)
	void moveSelectedCommand(int delta);

	// Highlight the command with the given index
	void selectCommand(int index);

	void onSave(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);

	void onMaxPlayCountEnabled(wxCommandEvent& ev);

	void onActorSelectionChanged(wxDataViewEvent& ev);
	void onCommandSelectionChanged(wxDataViewEvent& ev);

	void onAddActor(wxCommandEvent& ev);
	void onDeleteActor(wxCommandEvent& ev);
	void onValidateActors(wxCommandEvent& ev);
	void onActorEdited(wxDataViewEvent& ev);

	void onAddCommand(wxCommandEvent& ev);
	void onEditCommand(wxCommandEvent& ev);
	void onMoveUpCommand(wxCommandEvent& ev);
	void onMoveDownCommand(wxCommandEvent& ev);
	void onDeleteCommand(wxCommandEvent& ev);

	std::string removeMarkup(const std::string& input);
};

} // namespace ui
