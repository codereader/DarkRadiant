#pragma once

#include <map>

#include "ientity.h"
#include "icommandsystem.h"
#include "iradiant.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/dataview/TreeView.h"

#include "ConversationEntity.h"

namespace ui
{

class ConversationDialog;
typedef std::shared_ptr<ConversationDialog> ConversationDialogPtr;

/**
 * greebo: The conversation dialog is a modal top-level window providing
 * views and controls to facilitate the setup of inter-AI conversations.
 */
class ConversationDialog :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
private:
	// List of conversation_info entities
	conversation::ConvEntityColumns _convEntityColumns;
	wxutil::TreeModel::Ptr _entityList;
	wxutil::TreeView* _entityView;
	
	// List of conversations on the selected entity
	conversation::ConversationColumns _convColumns;
	wxutil::TreeModel::Ptr _convList;
	wxutil::TreeView* _convView;

	// Map of ConversationEntity objects, indexed by the name of the world entity
	conversation::ConversationEntityMap _entities;

	// Iterators for current entity and current objective
	conversation::ConversationEntityMap::iterator _curEntity;
	wxDataViewItem _currentConversation;

	wxButton* _addConvButton;
	wxButton* _editConvButton;
	wxButton* _deleteConvButton;
	wxButton* _moveUpConvButton;
	wxButton* _moveDownConvButton;
	wxButton* _clearConvButton;

	wxButton* _addEntityButton;
	wxButton* _deleteEntityButton;

public:
	ConversationDialog();

	// Command target to toggle the dialog
	static void ShowDialog(const cmd::ArgumentList& args);

	// Override DialogBase
	virtual int ShowModal();

private:
	// greebo: Saves the current working set to the entity
	void save();

	// Clears out all stored data
	void clear();

	void populateWidgets();

	// Re-loads the conversation from the selected entity
	void refreshConversationList();
	void updateConversationPanelSensitivity();
	void handleConversationSelectionChange();

	// WIDGET POPULATION
	void populateWindow();

	// Button callbacks
	void onOK(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);

	void onEntitySelectionChanged(wxDataViewEvent& ev);
	void onAddEntity(wxCommandEvent& ev);
	void onDeleteEntity(wxCommandEvent& ev);

	void onConversationSelectionChanged(wxDataViewEvent& ev);
	void onAddConversation(wxCommandEvent& ev);
	void onEditConversation(wxCommandEvent& ev);
	void onDeleteConversation(wxCommandEvent& ev);
	void onMoveConversationUpOrDown(wxCommandEvent& ev);
	void onClearConversations(wxCommandEvent& ev);

    void editSelectedConversation();
	int getSelectedConvIndex();
	void selectConvByIndex(int index);
};

} // namespace ui
