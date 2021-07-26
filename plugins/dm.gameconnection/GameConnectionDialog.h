#pragma once

#include "icommandsystem.h"

#include "wxutil/window/TransientWindow.h"
#include "wxutil/XmlResourceBasedWidget.h"

namespace ui
{

/**
 * stgatilov: This is top-level non-modal window
 * which displays the status of game connection system,
 * and allows to control its modes and actions.
 */
class GameConnectionDialog :
	public wxutil::TransientWindow,
	private wxutil::XmlResourceBasedWidget
{
private:
/*	wxButton* _addConvButton;
	wxButton* _editConvButton;
	wxButton* _deleteConvButton;
	wxButton* _moveUpConvButton;
	wxButton* _moveDownConvButton;
	wxButton* _clearConvButton;

	wxButton* _addEntityButton;
	wxButton* _deleteEntityButton;*/

public:
    // This is the actual home of the static instance
    static GameConnectionDialog& Instance();

    // Toggle the visibility of the dialog instance, constructing it if necessary.
    static void toggleDialog(const cmd::ArgumentList& args);

protected:
    // TransientWindow callbacks
    virtual void _preShow() override;
    virtual void _preHide() override;

private:
    GameConnectionDialog();

    
    /*	// greebo: Saves the current working set to the entity
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

	int getSelectedConvIndex();
	void selectConvByIndex(int index);*/
};

} // namespace ui
