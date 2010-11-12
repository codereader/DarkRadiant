#ifndef CLASSEDITOR_H_
#define CLASSEDITOR_H_

#include "SREntity.h"
#include "StimTypes.h"
#include <gtkmm/box.h>

namespace Gtk
{
	class SpinButton;
	class TreeView;
	class Entry;
	class Button;
	class ComboBox;
	class Label;
	class CheckButton;
}

namespace ui
{

class ClassEditor :
	public Gtk::VBox
{
protected:
	typedef std::map<Gtk::Entry*, std::string> EntryMap;
	EntryMap _entryWidgets;

	typedef std::map<Gtk::SpinButton*, std::string> SpinButtonMap;
	SpinButtonMap _spinWidgets;

	Gtk::TreeView* _list;

	// The entity object we're editing
	SREntityPtr _entity;

	// Helper class (owned by StimResponseEditor)
	StimTypes& _stimTypes;

	// TRUE if the GTK callbacks should be disabled
	bool _updatesDisabled;

	struct ListButtons
	{
		Gtk::Button* add;
		Gtk::Button* remove;
	} _listButtons;

	// The combo box to select the stim/response type
	struct TypeSelectorWidgets
	{
		Gtk::HBox* hbox;	// The box
		Gtk::ComboBox* list;	// the combo box
		Gtk::Label* label;	// The "Type:" label

		TypeSelectorWidgets() :
			hbox(NULL),
			list(NULL),
			label(NULL)
		{}
	};

	TypeSelectorWidgets _type;
	TypeSelectorWidgets _addType;

public:
	/** greebo: Constructs the shared widgets, but does not pack them
	 */
	ClassEditor(StimTypes& stimTypes);

	/** destructor
	 */
	virtual ~ClassEditor() {}

	/** greebo: Sets the new entity (is called by the subclasses)
	 */
	virtual void setEntity(const SREntityPtr& entity);

	/** greebo: Sets the given <key> of the current entity to <value>
	 */
	virtual void setProperty(const std::string& key, const std::string& value);

	/** greebo: Updates the widgets (must be implemented by the child classes)
	 */
	virtual void update() = 0;

protected:
	/** greebo: Creates the StimType Selector and buttons below the main list.
	 */
	virtual Gtk::Widget& createListButtons();

	/** greebo: Returns the name of the selected stim in the given combo box.
	 * 			The model behind that combo box has to be according to the
	 * 			one created by the StimTypes helper class.
	 */
	virtual std::string getStimTypeIdFromSelector(Gtk::ComboBox* widget);

	/** greebo: Adds/removes a S/R from the main list
	 */
	virtual void addSR() = 0;
	virtual void removeSR();

	/** greebo: Duplicates the currently selected S/R object
	 */
	void duplicateStimResponse();

	/** greebo: Returns the fabricated Stim Selector widget structure
	 */
	TypeSelectorWidgets createStimTypeSelector();

	/** greebo: Gets called when a check box is toggled, this should
	 * 			update the contents of possible associated entry fields.
	 */
	virtual void checkBoxToggled(Gtk::CheckButton* toggleButton) = 0;

	/** greebo: Gets called when an entry box changes, this can be
	 * 			overriden by the subclasses, if this is needed
	 */
	virtual void entryChanged(Gtk::Entry* entry);

	/** greebo: Gets called when a spin button changes, this can be
	 * 			overriden by the subclasses, if this is needed
	 */
	virtual void spinButtonChanged(Gtk::SpinButton* spinButton);

	/** greebo: Returns the ID of the currently selected stim/response
	 *
	 * @returns: the id (number) of the selected stim or -1 on failure
	 */
	int getIdFromSelection();

	/** greebo: Selects the given ID in the S/R list
	 */
	void selectId(int id);

	/** greebo: Gets called when the list selection changes
	 */
	virtual void selectionChanged() = 0;

	/** greebo: Opens the context menu. The treeview widget this event
	 * 			has been happening on gets passed so that the correct
	 * 			menu can be displayed (in the case of multiple possible treeviews).
	 */
	virtual void openContextMenu(Gtk::TreeView* view) = 0;

	// gtkmm Callback for Stim/Response selection changes
	void onSRSelectionChange();

	// The keypress handler for catching the keys in the treeview
	bool onTreeViewKeyPress(GdkEventKey* ev);

	// Release-event opens the context menu for right clicks
	bool onTreeViewButtonRelease(GdkEventButton* ev, Gtk::TreeView* view);

	// Gets called if any of the entry widget contents get changed
	void onEntryChanged(Gtk::Entry* entry);
	void onSpinButtonChanged(Gtk::SpinButton* spinButton);
	void onCheckboxToggle(Gtk::CheckButton* toggleButton);

	// Utility function to connect a checkbutton's "toggled" signal to "onCheckBoxToggle"
	void connectCheckButton(Gtk::CheckButton* checkButton);

	// Utility function to connect a spinbutton's "value-changed" signal to "onSpinButtonChanged"
	// It also associates the given spin button in the SpinButton map
	void connectSpinButton(Gtk::SpinButton* spinButton, const std::string& key);

	// Utility function to connect a entry's "changed" signal to "onEntryChanged"
	// It also associates the given entry in the EntryMap
	void connectEntry(Gtk::Entry* entry, const std::string& key);

	// Gets called on stim type selection change
	void onStimTypeSelect();
	void onAddTypeSelect();

	void onAddSR();
	void onRemoveSR();

	// Override/disable override menu items
	void onContextMenuEnable();
	void onContextMenuDisable();
	void onContextMenuDuplicate();
};

} // namespace ui

#endif /*CLASSEDITOR_H_*/
