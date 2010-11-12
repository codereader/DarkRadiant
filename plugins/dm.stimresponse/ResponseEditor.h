#ifndef RESPONSEEDITOR_H_
#define RESPONSEEDITOR_H_

#include <gtkmm/window.h>
#include "ClassEditor.h"

namespace Gtk
{
	class Menu;
	class MenuItem;
	class TreeView;
	class VBox;
	class CheckButton;
	class Entry;
	class SpinButton;
}

namespace ui
{

class ResponseEditor :
	public ClassEditor
{
private:
	struct ListContextMenu
	{
		Gtk::Menu* menu;
		Gtk::MenuItem* remove;
		Gtk::MenuItem* add;
		Gtk::MenuItem* enable;
		Gtk::MenuItem* disable;
		Gtk::MenuItem* duplicate;
	} _contextMenu;

	struct EffectWidgets
	{
		Gtk::TreeView* view;
		Gtk::Menu* contextMenu;
		Gtk::MenuItem* deleteMenuItem;
		Gtk::MenuItem* addMenuItem;
		Gtk::MenuItem* editMenuItem;
		Gtk::MenuItem* upMenuItem;
		Gtk::MenuItem* downMenuItem;
	} _effectWidgets;

	struct PropertyWidgets
	{
		Gtk::VBox* vbox;
		Gtk::CheckButton* active;
		Gtk::CheckButton* chanceToggle;
		Gtk::SpinButton* chanceEntry;
		Gtk::CheckButton* randomEffectsToggle;
		Gtk::Entry* randomEffectsEntry;
	} _propertyWidgets;

	Glib::RefPtr<Gtk::Window> _parent;

public:
	/** greebo: Constructor creates all the widgets
	 */
	ResponseEditor(const Glib::RefPtr<Gtk::Window>& parent, StimTypes& stimTypes);

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
	void checkBoxToggled(Gtk::CheckButton* toggleButton);

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
	Gtk::Widget& createEffectWidgets(); // Response effect list

	/** greebo: Gets called when the response selection gets changed
	 */
	virtual void selectionChanged();

	void openContextMenu(Gtk::TreeView* view);

	/** greebo: Creates all the widgets
	 */
	void populatePage();

	// Context menu GTK callbacks
	void onContextMenuAdd();
	void onContextMenuDelete();
	void onContextMenuEffectUp();
	void onContextMenuEffectDown();
	void onContextMenuEdit();

	void onEffectMenuDelete();
	void onEffectMenuEdit();
	void onEffectMenuAdd();
	void onEffectMenuEffectUp();
	void onEffectMenuEffectDown();

	// To catch double-clicks in the response effect list
	bool onEffectsViewButtonPress(GdkEventButton*);

	// Callback for Stim/Response and effect selection changes
	void onEffectSelectionChange();
};

} // namespace ui

#endif /*RESPONSEEDITOR_H_*/
