#include "SelectionSetToolmenu.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "idialogmanager.h"

#include "gtkutil/LeftAlignedLabel.h"
#include <gtkmm/box.h>
#include <gtkmm/comboboxentry.h>
#include <gtkmm/image.h>
#include <gtkmm/toolbutton.h>

namespace selection
{

	namespace
	{
		const char* const ENTRY_TOOLTIP = N_("Enter a name and hit ENTER to save a set.\n\n"
			"Select an item from the dropdown list to restore the selection.\n\n"
			"Hold SHIFT when opening the dropdown list and selecting the item to de-select the set.");
	}

SelectionSetToolmenu::SelectionSetToolmenu() :
	Gtk::ToolItem(),
	_listStore(Gtk::ListStore::create(_columns)),
	_clearSetsButton(NULL),
	_entry(Gtk::manage(new Gtk::ComboBoxEntry(_listStore, _columns.name)))
{
	// Hbox containing all our items
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 3));
	add(*hbox);

	// Pack Label
	hbox->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Selection Set: "))),
		false, false, 0);

	// Pack Combo Box
	hbox->pack_start(*_entry, true, true, 0);

	// Add tooltip
	_entry->set_tooltip_markup(_(ENTRY_TOOLTIP));

	// Add clear button
	{
		Gtk::Image* image = Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbufWithMask("delete.png")));
		image->show();

		_clearSetsButton = Gtk::manage(new Gtk::ToolButton(*image, _("Clear Selection Sets")));

		// Set tooltip
		_clearSetsButton->set_tooltip_text(_("Clear Selection Sets"));

		// Connect event
		_clearSetsButton->signal_clicked().connect(sigc::mem_fun(*this, &SelectionSetToolmenu::onDeleteAllSetsClicked));

		hbox->pack_start(*_clearSetsButton, false, false, 0);
	}

	// Connect the signals
	Gtk::Entry* childEntry = _entry->get_entry();
	childEntry->signal_activate().connect(sigc::mem_fun(*this, &SelectionSetToolmenu::onEntryActivated));

	_entry->signal_changed().connect(sigc::mem_fun(*this, &SelectionSetToolmenu::onSelectionChanged));

	// Populate the list
	update();

	// Add self as observer
	GlobalSelectionSetManager().addObserver(*this);

	show_all();
}

SelectionSetToolmenu::~SelectionSetToolmenu()
{
	GlobalSelectionSetManager().removeObserver(*this);
}

void SelectionSetToolmenu::onSelectionSetsChanged()
{
	update();
}

void SelectionSetToolmenu::update()
{
	// Clear all items from the treemodel first
	_listStore->clear();

	bool hasItems = false;

	GlobalSelectionSetManager().foreachSelectionSet([&] (const ISelectionSetPtr& set)
	{
		hasItems = true;

		Gtk::TreeModel::Row row = *_listStore->append();

		row[_columns.name] = set->getName();
	});

	// Tool button is sensitive if we have items in the list
	_clearSetsButton->set_sensitive(hasItems);
}

void SelectionSetToolmenu::onEntryActivated()
{
	// Create new selection set if possible
	std::string name = _entry->get_entry()->get_text();

	if (name.empty()) return;

	// don't create empty sets
	if (GlobalSelectionSystem().countSelected() == 0)
	{
		ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(
			_("Cannot create selection set"),
			_("Cannot create a selection set, there is nothing selected in the current scene."),
			ui::IDialog::MESSAGE_CONFIRM);

		dialog->run();
		return;
	}

	ISelectionSetPtr set = GlobalSelectionSetManager().createSelectionSet(name);

	assert(set != NULL);

	set->assignFromCurrentScene();

	// Clear the entry again
	_entry->get_entry()->set_text("");
}

void SelectionSetToolmenu::onSelectionChanged()
{
	std::string name = _entry->get_active_text();

	if (name.empty()) return;

	ISelectionSetPtr set = GlobalSelectionSetManager().findSelectionSet(name);

	if (set == NULL) return;

	// The user can choose to DESELECT the set nodes when holding down shift
	if ((GlobalEventManager().getModifierState() & GDK_SHIFT_MASK) != 0)
	{
		set->deselect();
	}
	else
	{
		set->select();
	}

	_entry->get_entry()->set_text("");
}

void SelectionSetToolmenu::onDeleteAllSetsClicked()
{
	ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(
		_("Delete all selection sets?"),
		_("This will delete all set definitions. The actual map objects will not be affected by this step.\n\nContinue with that operation?"),
		ui::IDialog::MESSAGE_ASK);

	ui::IDialog::Result result = dialog->run();

	if (result == ui::IDialog::RESULT_YES)
	{
		GlobalSelectionSetManager().deleteAllSelectionSets();
	}
}

} // namespace
