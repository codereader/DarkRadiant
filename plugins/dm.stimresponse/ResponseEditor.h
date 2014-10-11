#pragma once

#include "ClassEditor.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/TreeView.h"
#include <memory>

namespace ui
{

class ResponseEditor :
	public ClassEditor,
	private wxutil::XmlResourceBasedWidget
{
private:
	struct ListContextMenu
	{
		std::unique_ptr<wxMenu> menu;
		wxMenuItem* remove;
		wxMenuItem* add;
		wxMenuItem* enable;
		wxMenuItem* disable;
		wxMenuItem* duplicate;
	} _contextMenu;

	struct EffectWidgets
	{
		wxutil::TreeView* view;
		std::unique_ptr<wxMenu> contextMenu;
		wxMenuItem* deleteMenuItem;
		wxMenuItem* addMenuItem;
		wxMenuItem* editMenuItem;
		wxMenuItem* upMenuItem;
		wxMenuItem* downMenuItem;
	} _effectWidgets;

	struct PropertyWidgets
	{
		wxCheckBox* active;
		wxCheckBox* chanceToggle;
		wxSpinCtrlDouble* chanceEntry;
		wxCheckBox* randomEffectsToggle;
		wxTextCtrl* randomEffectsEntry;
	} _propertyWidgets;

public:
	/** greebo: Constructor creates all the widgets
	 */
	ResponseEditor(wxWindow* parent, StimTypes& stimTypes);

	/** greebo: Sets the new entity (updates the treeviews)
	 */
	virtual void setEntity(const SREntityPtr& entity);

	/** greebo: Updates the widgets (e.g. after a selection change)
	 */
	void update();

private:
	/** greebo: Updates the associated text fields when a check box
	 * 			is toggled.
	 */
	void checkBoxToggled(wxCheckBox* toggleButton);

	/** greebo: Adds a new response effect to the list.
	 */
	void addEffect();

	/** greebo: Removes the currently selected response effect
	 */
	void removeEffect();

	/** greebo: Edits the currently selected effect
	 */
	void editEffect();

	/** greebo: Moves the selected effect up or down (i.e. increasing
	 * 			or decreasing its index).
	 *
	 * @direction: +1 for moving it down (increasing the index)
	 * 			   -1 for moving it up (decreasing the index)
	 */
	void moveEffect(int direction);

	/** greebo: Updates the sensitivity of the effects context menu
	 */
	void updateEffectContextMenu();

	/** greebo: Selects the effect with the given index in the treeview.
	 */
	void selectEffectIndex(const unsigned int index);

	/** greebo: Returns the ID of the currently selected response effect
	 *
	 * @returns: the index of the selected effect or -1 on failure
	 */
	int getEffectIdFromSelection();

	/** greebo: Adds a new default response to the entity
	 */
	void addSR();

	// Widget creator helpers
	void createContextMenu();
	void createEffectWidgets(); // Response effect list

	/** greebo: Gets called when the response selection gets changed
	 */
	virtual void selectionChanged();

	void openContextMenu(wxutil::TreeView* view);

	/** greebo: Creates all the widgets
	 */
	void populatePage(wxWindow* parent);

	// Context menu callbacks
	void onContextMenuAdd(wxCommandEvent& ev);
	void onContextMenuDelete(wxCommandEvent& ev);
	void onContextMenuEffectUp(wxCommandEvent& ev);
	void onContextMenuEffectDown(wxCommandEvent& ev);
	void onContextMenuEdit(wxCommandEvent& ev);

	void onEffectMenuDelete(wxCommandEvent& ev);
	void onEffectMenuEdit(wxCommandEvent& ev);
	void onEffectMenuAdd(wxCommandEvent& ev);
	void onEffectMenuEffectUp(wxCommandEvent& ev);
	void onEffectMenuEffectDown(wxCommandEvent& ev);

	// To catch double-clicks in the response effect list
	void onEffectItemActivated(wxDataViewEvent& ev);

	// Callback for Stim/Response and effect selection changes
	void onEffectSelectionChange(wxDataViewEvent& ev);
};

} // namespace ui
