#pragma once

#include "StimTypes.h"
#include "SREntity.h"
#include <wx/panel.h>
#include "gtkutil/TreeModelFilter.h"
#include "gtkutil/TreeView.h"
#include <memory>

class wxTextCtrl;
class wxStaticText;
class wxButton;
class wxMenu;
class wxMenuItem;
class wxBoxSizer;

namespace ui 
{

class CustomStimEditor :
	public wxPanel
{
	struct PropertyWidget
	{
		wxPanel* vbox;
		wxStaticText* nameLabel;
		wxTextCtrl* nameEntry;
	} _propertyWidgets;

	struct ListContextMenu {
		std::unique_ptr<wxMenu> menu;
		wxMenuItem* remove;
		wxMenuItem* add;
	} _contextMenu;

	struct ListButtons
	{
		wxButton* add;
		wxButton* remove;
	} _listButtons;

	// The filtered liststore
	wxutil::TreeModel* _customStimStore;

	// The treeview and its selection
	wxutil::TreeView* _list;

	// Reference to the helper object (owned by StimResponseEditor)
	StimTypes& _stimTypes;

	// To avoid GTK callback loops
	bool _updatesDisabled;

	// The entity we're working on
	SREntityPtr _entity;

public:
	/** greebo: Constructor creates all the widgets
	 */
	CustomStimEditor(wxWindow* parent, StimTypes& stimTypes);

	/** greebo: Sets the new entity (is called by the subclasses)
	 */
	void setEntity(const SREntityPtr& entity);

private:
	/** greebo: Updates the property widgets on selection change
	 */
	void update();

	/** greebo: Returns the ID of the currently selected stim type
	 *
	 * @returns: the id (number) of the selected stim or -1 on failure
	 */
	int getIdFromSelection();

	/** greebo: Selects the given ID in the stim type list
	 */
	void selectId(int id);

	/** greebo: Adds/removes a (selected) stim type
	 */
	void addStimType();
	void removeStimType();

	/** greebo: Widget creators
	 */
	void createContextMenu();
	wxBoxSizer* createListButtons();

	/** greebo: Creates all the widgets
	 */
	void populatePage();

	// Callbacks
	void onAddStimType(wxCommandEvent& ev);
	void onRemoveStimType(wxCommandEvent& ev);
	void onEntryChanged(wxCommandEvent& ev);
	void onSelectionChange(wxDataViewEvent& ev);
	void onContextMenu(wxDataViewEvent& ev);

	void onContextMenuAdd(wxCommandEvent& ev);
	void onContextMenuDelete(wxCommandEvent& ev);
};

} // namespace ui
