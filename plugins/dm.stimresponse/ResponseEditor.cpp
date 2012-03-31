#include "ResponseEditor.h"

#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/TreeModel.h"
#include "string/convert.h"

#include "i18n.h"
#include "EffectEditor.h"

#include <gtkmm/treeview.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/combobox.h>
#include <gtkmm/entry.h>
#include <gtkmm/table.h>
#include <gtkmm/stock.h>

namespace ui {

	namespace
	{
		const char* const LABEL_RESPONSE_EFFECTS = N_("Response Effects");
	}

ResponseEditor::ResponseEditor(const Glib::RefPtr<Gtk::Window>& parent, StimTypes& stimTypes) :
	ClassEditor(stimTypes),
	_parent(parent)
{
	populatePage();
	createContextMenu();
}

void ResponseEditor::setEntity(const SREntityPtr& entity)
{
	// Pass the call to the base class
	ClassEditor::setEntity(entity);

	if (entity != NULL)
	{
		_list->set_model(_entity->getResponseStore());

		// Clear the treeview (unset the model)
		_effectWidgets.view->unset_model();
	}
}

void ResponseEditor::update()
{
	_updatesDisabled = true;

	int id = getIdFromSelection();

	if (id > 0 && _entity != NULL)
	{
		_propertyWidgets.vbox->set_sensitive(true);

		StimResponse& sr = _entity->get(id);

		// Get the iter into the liststore pointing at the correct STIM_YYYY type
		_type.list->set_active(_stimTypes.getIterForName(sr.get("type")));

		// Active
		_propertyWidgets.active->set_active(sr.get("state") == "1");

		// Use Radius
		bool useRandomEffects = (sr.get("random_effects") != "");
		_propertyWidgets.randomEffectsToggle->set_active(useRandomEffects);
		_propertyWidgets.randomEffectsEntry->set_text(sr.get("random_effects"));
		_propertyWidgets.randomEffectsEntry->set_sensitive(useRandomEffects);

		// Use Chance
		bool useChance = (sr.get("chance") != "");
		_propertyWidgets.chanceToggle->set_active(useChance);
		_propertyWidgets.chanceEntry->set_value(string::convert<float>(sr.get("chance")));
		_propertyWidgets.chanceEntry->set_sensitive(useChance);

		_effectWidgets.view->set_model(sr.updateAndGetEffectStore());

		// Disable the editing of inherited properties completely
		if (sr.inherited())
		{
			_propertyWidgets.vbox->set_sensitive(false);
		}

		// Update the delete context menu item
		_contextMenu.remove->set_sensitive(!sr.inherited());

		// If there is anything selected, the duplicate item is always active
		_contextMenu.duplicate->set_sensitive(true);

		// Update the "enable/disable" menu items
		bool state = sr.get("state") == "1";
		_contextMenu.enable->set_sensitive(!state);
		_contextMenu.disable->set_sensitive(state);

		// The response effect list may be empty, so force an update of the
		// context menu sensitivity, in the case the "selection changed"
		// signal doesn't get called
		updateEffectContextMenu();
	}
	else
	{
		// Nothing selected
		_propertyWidgets.vbox->set_sensitive(false);
		// Clear the effect tree view
		_effectWidgets.view->unset_model();

		_contextMenu.enable->set_sensitive(false);
		_contextMenu.disable->set_sensitive(false);
		_contextMenu.remove->set_sensitive(false);
		_contextMenu.duplicate->set_sensitive(false);
	}

	_updatesDisabled = false;
}

void ResponseEditor::populatePage()
{
	Gtk::HBox* srHBox = Gtk::manage(new Gtk::HBox(false, 12));
	pack_start(*srHBox, true, true, 0);

	// List and buttons below
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));
	vbox->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_list)), true, true, 0);

	// Create the type selector plus buttons and pack them
	vbox->pack_start(createListButtons(), false, false, 0);

	srHBox->pack_start(*vbox, false, false, 0);

	// Response property section
	_propertyWidgets.vbox = Gtk::manage(new Gtk::VBox(false, 6));
	srHBox->pack_start(*_propertyWidgets.vbox, true, true, 0);

	_type = createStimTypeSelector();
	_propertyWidgets.vbox->pack_start(*_type.hbox, false, false, 0);
	_type.list->signal_changed().connect(sigc::mem_fun(*this, &ResponseEditor::onStimTypeSelect));

	// Create the table for the widget alignment
	Gtk::Table* table = Gtk::manage(new Gtk::Table(3, 2, false));
	table->set_row_spacings(6);
	table->set_col_spacings(6);
	_propertyWidgets.vbox->pack_start(*table, false, false, 0);

	// Active
	_propertyWidgets.active = Gtk::manage(new Gtk::CheckButton(_("Active")));
	table->attach(*_propertyWidgets.active, 0, 2, 0, 1);

	// Random Effects Toggle
	_propertyWidgets.randomEffectsToggle = Gtk::manage(new Gtk::CheckButton(_("Random Effects:")));
	_propertyWidgets.randomEffectsEntry = Gtk::manage(new Gtk::Entry);

	table->attach(*_propertyWidgets.randomEffectsToggle, 0, 1, 2, 3, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*_propertyWidgets.randomEffectsEntry, 1, 2, 2, 3);

	// Chance variable
	_propertyWidgets.chanceToggle = Gtk::manage(new Gtk::CheckButton(_("Chance:")));
	_propertyWidgets.chanceEntry = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 1.0, 0.1)), 0, 2));

	table->attach(*_propertyWidgets.chanceToggle, 0, 1, 1, 2, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*_propertyWidgets.chanceEntry, 1, 2, 1, 2);

	// Connect the signals
	connectCheckButton(_propertyWidgets.active);
	connectCheckButton(_propertyWidgets.randomEffectsToggle);
	connectCheckButton(_propertyWidgets.chanceToggle);

	connectEntry(_propertyWidgets.randomEffectsEntry, "random_effects");

	connectSpinButton(_propertyWidgets.chanceEntry, "chance");

	_propertyWidgets.vbox->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _(LABEL_RESPONSE_EFFECTS) + "</b>")),
		false, false, 0
	);

	_propertyWidgets.vbox->pack_start(createEffectWidgets(), true, true, 0);
}

// Create the response effect list widgets
Gtk::Widget& ResponseEditor::createEffectWidgets()
{
	_effectWidgets.view = Gtk::manage(new Gtk::TreeView);
	_effectWidgets.view->set_size_request(-1, 150);

	// Connect the signals
	_effectWidgets.view->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &ResponseEditor::onEffectSelectionChange));

	_effectWidgets.view->signal_button_press_event().connect(
		sigc::mem_fun(*this, &ResponseEditor::onEffectsViewButtonPress), false);

	_effectWidgets.view->signal_button_release_event().connect(
		sigc::bind(sigc::mem_fun(*this, &ResponseEditor::onTreeViewButtonRelease), _effectWidgets.view));

	_effectWidgets.view->append_column(
		*Gtk::manage(new gtkutil::TextColumn("#", StimResponse::getColumns().index)));

	_effectWidgets.view->append_column(
		*Gtk::manage(new gtkutil::TextColumn(_("Effect"), StimResponse::getColumns().caption)));

	_effectWidgets.view->append_column(
		*Gtk::manage(new gtkutil::TextColumn(_("Details (double-click to edit)"), StimResponse::getColumns().arguments)));

	// Return the tree view in a frame
	return *Gtk::manage(new gtkutil::ScrolledFrame(*_effectWidgets.view));
}

void ResponseEditor::checkBoxToggled(Gtk::CheckButton* toggleButton)
{
	bool active = toggleButton->get_active();

	if (toggleButton == _propertyWidgets.active)
	{
		setProperty("state", active ? "1" : "0");
	}
	else if (toggleButton == _propertyWidgets.randomEffectsToggle)
	{
		std::string entryText = _propertyWidgets.randomEffectsEntry->get_text();

		// Enter a default value for the entry text, if it's empty up till now.
		if (active)
		{
			entryText += (entryText.empty()) ? "1" : "";
		}
		else {
			entryText = "";
		}

		setProperty("random_effects", entryText);
	}
	else if (toggleButton == _propertyWidgets.chanceToggle)
	{
		std::string entryText = string::to_string(_propertyWidgets.chanceEntry->get_value());

		setProperty("chance", active ? entryText : "");
	}
}

void ResponseEditor::addEffect()
{
	if (_entity == NULL) return;

	int id = getIdFromSelection();

	if (id > 0)
	{
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();

		// Make sure we have a response
		if (sr.get("class") == "R") {
			// Add a new effect and update all the widgets
			sr.addEffect(effectIndex);
			update();
		}
	}
}

void ResponseEditor::removeEffect()
{
	if (_entity == NULL) return;

	int id = getIdFromSelection();

	if (id > 0)
	{
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();

		// Make sure we have a response and anything selected
		if (sr.get("class") == "R" && effectIndex > 0)
		{
			// Remove the effect and update all the widgets
			sr.deleteEffect(effectIndex);
			update();
		}
	}
}

void ResponseEditor::editEffect()
{
	if (_entity == NULL) return;

	int id = getIdFromSelection();

	if (id > 0)
	{
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();

		// Make sure we have a response and anything selected
		if (sr.get("class") == "R" && effectIndex > 0)
		{
			// Create a new effect editor (self-destructs)
			new EffectEditor(_parent, sr, effectIndex, _stimTypes, *this);

			// The editor is modal and will destroy itself, our work is done
		}
	}
}

void ResponseEditor::moveEffect(int direction)
{
	if (_entity == NULL) return;

	int id = getIdFromSelection();

	if (id > 0)
	{
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();

		if (sr.get("class") == "R" && effectIndex > 0)
		{
			// Move the index (swap the specified indices)
			sr.moveEffect(effectIndex, effectIndex + direction);
			update();
			// Select the moved effect after the update
			selectEffectIndex(effectIndex + direction);
		}
	}
}

void ResponseEditor::updateEffectContextMenu()
{
	// Check if we have anything selected at all
	int curEffectIndex = getEffectIdFromSelection();
	int highestEffectIndex = 0;

	bool anythingSelected = curEffectIndex >= 0;

	int srId = getIdFromSelection();
	if (srId > 0) {
		StimResponse& sr = _entity->get(srId);
		highestEffectIndex = sr.highestEffectIndex();
	}

	bool upActive = anythingSelected && curEffectIndex > 1;
	bool downActive = anythingSelected && curEffectIndex < highestEffectIndex;

	// Enable or disable the "Delete" context menu items based on the presence
	// of a selection.
	_effectWidgets.deleteMenuItem->set_sensitive(anythingSelected);
	_effectWidgets.editMenuItem->set_sensitive(anythingSelected);

	_effectWidgets.upMenuItem->set_sensitive(upActive);
	_effectWidgets.downMenuItem->set_sensitive(downActive);
}

// Create the context menus
void ResponseEditor::createContextMenu()
{
	// Menu widgets
	_contextMenu.menu = Gtk::manage(new Gtk::Menu);

	// Each menu gets a delete item
	_contextMenu.remove = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::DELETE, _("Delete")));
	//_contextMenu.add = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::ADD, "Add"));
	_contextMenu.enable = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::YES, _("Activate")));
	_contextMenu.disable = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::NO, _("Deactivate")));
	_contextMenu.duplicate = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::COPY, _("Duplicate")));

	//_contextMenu.menu->append(*_contextMenu.add);
	_contextMenu.menu->append(*_contextMenu.enable);
	_contextMenu.menu->append(*_contextMenu.disable);
	_contextMenu.menu->append(*_contextMenu.duplicate);
	_contextMenu.menu->append(*_contextMenu.remove);

	_effectWidgets.contextMenu = Gtk::manage(new Gtk::Menu);

	_effectWidgets.addMenuItem = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::ADD,
															   _("Add new Effect")));
	_effectWidgets.editMenuItem = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::EDIT,
															   _("Edit")));
	_effectWidgets.deleteMenuItem = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::DELETE,
															   _("Delete")));
	_effectWidgets.upMenuItem = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::GO_UP,
															   _("Move Up")));
	_effectWidgets.downMenuItem = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::GO_DOWN,
															   _("Move Down")));

	_effectWidgets.contextMenu->append(*_effectWidgets.addMenuItem);
	_effectWidgets.contextMenu->append(*_effectWidgets.editMenuItem);
	_effectWidgets.contextMenu->append(*_effectWidgets.upMenuItem);
	_effectWidgets.contextMenu->append(*_effectWidgets.downMenuItem);
	_effectWidgets.contextMenu->append(*_effectWidgets.deleteMenuItem);

	// Connect up the signals
	_contextMenu.remove->signal_activate().connect(sigc::mem_fun(*this, &ResponseEditor::onContextMenuDelete));

	/*_contextMenu.add->signal_activate().connect(sigc::mem_fun(*this, &ResponseEditor::onContextMenuAdd));*/
	_contextMenu.enable->signal_activate().connect(sigc::mem_fun(*this, &ResponseEditor::onContextMenuEnable));
	_contextMenu.disable->signal_activate().connect(sigc::mem_fun(*this, &ResponseEditor::onContextMenuDisable));
	_contextMenu.duplicate->signal_activate().connect(sigc::mem_fun(*this, &ResponseEditor::onContextMenuDuplicate));

	_effectWidgets.deleteMenuItem->signal_activate().connect(sigc::mem_fun(*this, &ResponseEditor::onEffectMenuDelete));
	_effectWidgets.editMenuItem->signal_activate().connect(sigc::mem_fun(*this, &ResponseEditor::onEffectMenuEdit));
	_effectWidgets.addMenuItem->signal_activate().connect(sigc::mem_fun(*this, &ResponseEditor::onEffectMenuAdd));
	_effectWidgets.upMenuItem->signal_activate().connect(sigc::mem_fun(*this, &ResponseEditor::onEffectMenuEffectUp));
	_effectWidgets.downMenuItem->signal_activate().connect(sigc::mem_fun(*this, &ResponseEditor::onEffectMenuEffectDown));

	// Show menus (not actually visible until popped up)
	_contextMenu.menu->show_all();
	_effectWidgets.contextMenu->show_all();
}

void ResponseEditor::selectEffectIndex(const unsigned int index)
{
	// Setup the selectionfinder to search for the index string
	gtkutil::TreeModel::SelectionFinder finder(index, StimResponse::getColumns().index.index());

	_effectWidgets.view->get_model()->foreach_iter(
		sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	if (finder.getIter())
	{
		// Set the active row of the list to the given effect
		_effectWidgets.view->get_selection()->select(finder.getIter());
	}
}

int ResponseEditor::getEffectIdFromSelection()
{
	Gtk::TreeModel::iterator iter = _effectWidgets.view->get_selection()->get_selected();

	if (iter && _entity != NULL)
	{
		return (*iter)[StimResponse::getColumns().index];
	}
	else
	{
		return -1;
	}
}

void ResponseEditor::openContextMenu(Gtk::TreeView* view)
{
	// Check the treeview this remove call is targeting
	if (view == _list)
	{
		_contextMenu.menu->popup(1, gtk_get_current_event_time());
	}
	else if (view == _effectWidgets.view)
	{
		_effectWidgets.contextMenu->popup(1, gtk_get_current_event_time());
	}
}

void ResponseEditor::selectionChanged()
{
	update();
}

void ResponseEditor::addSR()
{
	if (_entity == NULL) return;

	// Create a new StimResponse object
	int id = _entity->add();

	// Get a reference to the newly allocated object
	StimResponse& sr = _entity->get(id);
	sr.set("class", "R");

	// Get the selected stim type name from the combo box
	std::string name = getStimTypeIdFromSelector(_addType.list);
	sr.set("type", (!name.empty()) ? name : _stimTypes.getFirstName());

	sr.set("state", "1");

	// Update the list stores AFTER the type has been set
	_entity->updateListStores();

	// Select the newly created response
	selectId(id);
}

// Button click events on TreeViews
bool ResponseEditor::onEffectsViewButtonPress(GdkEventButton* ev)
{
	if (ev->type == GDK_2BUTTON_PRESS)
	{
		// Call the effect editor upon double click
		editEffect();
		return true;
	}

	return false;
}

void ResponseEditor::onEffectMenuDelete()
{
	removeEffect();
}

void ResponseEditor::onEffectMenuEdit()
{
	editEffect();
}

void ResponseEditor::onEffectMenuAdd()
{
	addEffect();
}

void ResponseEditor::onEffectMenuEffectUp()
{
	moveEffect(-1);
}

void ResponseEditor::onEffectMenuEffectDown()
{
	moveEffect(+1);
}

// Delete context menu items activated
void ResponseEditor::onContextMenuDelete()
{
	// Delete the selected stim from the list
	removeSR();
}

// Delete context menu items activated
void ResponseEditor::onContextMenuAdd()
{
	addSR();
}

// Callback for effects treeview selection changes
void ResponseEditor::onEffectSelectionChange()
{
	if (_updatesDisabled)	return; // Callback loop guard

	// Update the sensitivity
	updateEffectContextMenu();
}

} // namespace ui
