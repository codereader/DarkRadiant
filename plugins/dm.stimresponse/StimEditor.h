#ifndef STIMEDITOR_H_
#define STIMEDITOR_H_

#include "ClassEditor.h"

namespace Gtk
{
	class VBox;
	class HBox;
	class CheckButton;
	class SpinButton;
	class Label;
	class Menu;
	class Entry;
	class MenuItem;
}

namespace ui
{

class StimEditor :
	public ClassEditor
{
	struct PropertyWidgets
	{
		Gtk::VBox* vbox;
		Gtk::CheckButton* active;
		Gtk::CheckButton* useBounds;
		Gtk::CheckButton* radiusToggle;
		Gtk::SpinButton* radiusEntry;
		Gtk::CheckButton* finalRadiusToggle;
		Gtk::SpinButton* finalRadiusEntry;
		Gtk::CheckButton* timeIntToggle;
		Gtk::SpinButton* timeIntEntry;
		Gtk::Label* timeUnitLabel;

		struct TimerWidgets
		{
			Gtk::CheckButton* toggle;
			Gtk::HBox* entryHBox;
			Gtk::SpinButton* hour;
			Gtk::SpinButton* minute;
			Gtk::SpinButton* second;
			Gtk::SpinButton* millisecond;

			Gtk::CheckButton* typeToggle;

			Gtk::HBox* reloadHBox;
			Gtk::CheckButton* reloadToggle;
			Gtk::SpinButton* reloadEntry;
			Gtk::Label* reloadLabel;

			Gtk::CheckButton* waitToggle;
		} timer;

		Gtk::CheckButton* durationToggle;
		Gtk::SpinButton* durationEntry;
		Gtk::Label* durationUnitLabel;
		Gtk::CheckButton* maxFireCountToggle;
		Gtk::SpinButton* maxFireCountEntry;
		Gtk::CheckButton* magnToggle;
		Gtk::SpinButton* magnEntry;
		Gtk::CheckButton* falloffToggle;
		Gtk::SpinButton* falloffEntry;
		Gtk::CheckButton* chanceToggle;
		Gtk::SpinButton* chanceEntry;
		Gtk::CheckButton* velocityToggle;
		Gtk::Entry* velocityEntry;

		struct BoundsWidgets
		{
			Gtk::CheckButton* toggle;
			Gtk::HBox* hbox;
			Gtk::Label* minLabel;
			Gtk::Entry* minEntry;
			Gtk::Label* maxLabel;
			Gtk::Entry* maxEntry;
		} bounds;
	} _propertyWidgets;

	struct ListContextMenu
	{
		boost::shared_ptr<Gtk::Menu> menu;
		Gtk::MenuItem* remove;
		Gtk::MenuItem* add;
		Gtk::MenuItem* enable;
		Gtk::MenuItem* disable;
		Gtk::MenuItem* duplicate;
	} _contextMenu;

public:
	/** greebo: Constructor creates all the widgets
	 */
	StimEditor(StimTypes& stimTypes);

	/** greebo: Sets the new entity (is called by the StimResponseEditor class)
	 */
	virtual void setEntity(const SREntityPtr& entity);

	/** greebo: Updates the widgets (e.g. after a selection change)
	 */
	void update();

private:
	/** greebo: Retrieves the formatted timer string h:m:s:ms
	 */
	std::string getTimerString();

	/** greebo: Adds a new stim to the list
	 */
	void addSR();

	/** greebo: Gets called when a spinbutton changes, overrides the
	 * 			method from the base class.
	 */
	void spinButtonChanged(Gtk::SpinButton* spinButton);

	/** greebo: Updates the associated text fields when a check box
	 * 			is toggled.
	 */
	void checkBoxToggled(Gtk::CheckButton* toggleButton);

	/** greebo: As the name states, this creates the context menu widgets.
	 */
	void createContextMenu();

	/** greebo: Widget creation helper methods
	 */
	Gtk::Widget& createPropertyWidgets();

	/** greebo: Gets called when the stim selection gets changed
	 */
	virtual void selectionChanged();

	void openContextMenu(Gtk::TreeView* view);

	/** greebo: Creates all the widgets
	 */
	void populatePage();

	// Context menu GTK callbacks
	void onContextMenuAdd();
	void onContextMenuDelete();
};

} // namespace ui

#endif /*STIMEDITOR_H_*/
