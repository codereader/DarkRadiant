#ifndef CUSTOMSTIMEDITOR_H_
#define CUSTOMSTIMEDITOR_H_

#include "StimTypes.h"
#include "SREntity.h"
#include <gtkmm/box.h>
#include <gtkmm/treemodelfilter.h>

namespace Gtk
{
	class Entry;
	class VBox;
	class Label;
	class Menu;
	class MenuItem;
	class Button;
	class TreeView;
}

namespace ui {

class CustomStimEditor :
	public Gtk::HBox
{
	struct PropertyWidget
	{
		Gtk::VBox* vbox;
		Gtk::Label* nameLabel;
		Gtk::Entry* nameEntry;
	} _propertyWidgets;

	struct ListContextMenu {
		Gtk::Menu* menu;
		Gtk::MenuItem* remove;
		Gtk::MenuItem* add;
	} _contextMenu;

	struct ListButtons
	{
		Gtk::Button* add;
		Gtk::Button* remove;
	} _listButtons;

	// The filtered liststore (a GtkTreeModelFilter)
	Glib::RefPtr<Gtk::TreeModelFilter> _customStimStore;

	// The treeview and its selection
	Gtk::TreeView* _list;

	// Reference to the helper object (owned by StimResponseEditor)
	StimTypes& _stimTypes;

	// To avoid GTK callback loops
	bool _updatesDisabled;

	// The entity we're working on
	SREntityPtr _entity;

public:
	/** greebo: Constructor creates all the widgets
	 */
	CustomStimEditor(StimTypes& stimTypes);

	/** greebo: Sets the new entity (is called by the subclasses)
	 */
	void setEntity(const SREntityPtr& entity);

private:
	/** greebo: Updates the property widgets on selection change
	 */
	void update();

	/** greebo: Gets called when an entry box changes, this can be
	 * 			overriden by the subclasses, if this is needed
	 */
	void entryChanged(Gtk::Entry* editable);

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
	Gtk::Widget& createListButtons();

	/** greebo: Creates all the widgets
	 */
	void populatePage();

	// gtkmm Callbacks
	void onAddStimType();
	void onRemoveStimType();
	void onEntryChanged();
	void onSelectionChange();

	// Context menu
	// Release-event opens the context menu for right clicks
	bool onTreeViewButtonRelease(GdkEventButton* ev);

	void onContextMenuAdd();
	void onContextMenuDelete();
};

} // namespace ui

#endif /*CUSTOMSTIMEDITOR_H_*/
