#include "CustomStimEditor.h"

#include "idialogmanager.h"

#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/StockIconMenuItem.h"
#include "string/convert.h"
#include "i18n.h"

#include <gtkmm/treeview.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/entry.h>
#include <gtkmm/image.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/label.h>

namespace ui {

	namespace
	{
		const int TREE_VIEW_WIDTH = 250;
		const int TREE_VIEW_HEIGHT = 200;
	}

/** greebo: Constructor creates all the widgets
 */
CustomStimEditor::CustomStimEditor(StimTypes& stimTypes) :
	Gtk::HBox(false, 12),
	_stimTypes(stimTypes),
	_updatesDisabled(false)
{
	populatePage();

	// Setup the context menu items and connect them to the callbacks
	createContextMenu();

	// The list may be empty, update the sensitivity
	update();

	show_all();
}

void CustomStimEditor::setEntity(const SREntityPtr& entity)
{
	_entity = entity;
}

/** greebo: As the name states, this creates the context menu widgets.
 */
void CustomStimEditor::createContextMenu()
{
	// Menu widgets
	_contextMenu.menu = Gtk::manage(new Gtk::Menu);

	// Each menu gets a delete item
	_contextMenu.remove = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::DELETE, _("Delete")));
	_contextMenu.add = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::ADD, _("Add")));

	_contextMenu.menu->append(*_contextMenu.add);
	_contextMenu.menu->append(*_contextMenu.remove);

	// Connect up the signals
	_contextMenu.remove->signal_activate().connect(sigc::mem_fun(*this, &CustomStimEditor::onContextMenuDelete));
	_contextMenu.add->signal_activate().connect(sigc::mem_fun(*this, &CustomStimEditor::onContextMenuAdd));

	// Show menus (not actually visible until popped up)
	_contextMenu.menu->show_all();
}

/** greebo: Creates all the widgets
 */
void CustomStimEditor::populatePage()
{
	set_border_width(6);

	// Setup a treemodel filter to display the custom stims only
	_customStimStore = Gtk::TreeModelFilter::create(_stimTypes.getListStore());
	_customStimStore->set_visible_column(_stimTypes.getColumns().isCustom);

	_list = Gtk::manage(new Gtk::TreeView(_customStimStore));
	_list->set_size_request(TREE_VIEW_WIDTH, TREE_VIEW_HEIGHT);

	// Connect the signals to the callbacks
	_list->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &CustomStimEditor::onSelectionChange));
	_list->signal_button_release_event().connect(sigc::mem_fun(*this, &CustomStimEditor::onTreeViewButtonRelease));

	// Add the columns to the treeview
	// ID number
	Gtk::TreeViewColumn* numCol = Gtk::manage(new Gtk::TreeViewColumn(_("ID")));

	Gtk::CellRendererText* numRenderer = Gtk::manage(new Gtk::CellRendererText);
	numCol->pack_start(*numRenderer, false);
	numCol->add_attribute(numRenderer->property_text(), _stimTypes.getColumns().id);

	_list->append_column(*numCol);

	// The Type
	Gtk::TreeViewColumn* typeCol = Gtk::manage(new Gtk::TreeViewColumn(_("Type")));

	Gtk::CellRendererPixbuf* typeIconRenderer = Gtk::manage(new Gtk::CellRendererPixbuf);
	typeCol->pack_start(*typeIconRenderer, false);

	Gtk::CellRendererText* typeTextRenderer = Gtk::manage(new Gtk::CellRendererText);
	typeCol->pack_start(*typeTextRenderer, false);

	typeCol->add_attribute(typeTextRenderer->property_text(), _stimTypes.getColumns().caption);
	typeCol->add_attribute(typeIconRenderer->property_pixbuf(), _stimTypes.getColumns().icon);

	_list->append_column(*typeCol);

	Gtk::VBox* listVBox = Gtk::manage(new Gtk::VBox(false, 6));
	listVBox->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_list)), true, true, 0);
	listVBox->pack_start(createListButtons(), false, false, 0);

	pack_start(*listVBox, false, false, 0);

	_propertyWidgets.vbox = Gtk::manage(new Gtk::VBox(false, 6));

	pack_start(*_propertyWidgets.vbox, true, true, 0);

	// The name widgets
	Gtk::HBox* nameHBox = Gtk::manage(new Gtk::HBox(false, 6));
	_propertyWidgets.nameLabel = Gtk::manage(new Gtk::Label(_("Name:")));
	_propertyWidgets.nameEntry = Gtk::manage(new Gtk::Entry);

	nameHBox->pack_start(*_propertyWidgets.nameLabel, false, false, 0);
	nameHBox->pack_start(*_propertyWidgets.nameEntry, true, true, 0);

	// Connect the entry field
	_propertyWidgets.nameEntry->signal_changed().connect(sigc::mem_fun(*this, &CustomStimEditor::onEntryChanged));

	_propertyWidgets.vbox->pack_start(*nameHBox, false, false, 0);

	Gtk::Label* infoText = Gtk::manage(new gtkutil::LeftAlignedLabel(
		_("<b>Note:</b> Please beware that deleting custom stims may\n"
		"affect other entities as well. So check before you delete.")
	));
	_propertyWidgets.vbox->pack_start(*infoText, false, false, 0);
}

Gtk::Widget& CustomStimEditor::createListButtons()
{
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(true, 6));

	_listButtons.add = Gtk::manage(new Gtk::Button(_("Add Stim Type")));
	_listButtons.add->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::ADD, Gtk::ICON_SIZE_BUTTON)));

	_listButtons.remove = Gtk::manage(new Gtk::Button(_("Remove Stim Type")));
	_listButtons.remove->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON)));

	hbox->pack_start(*_listButtons.add, true, true, 0);
	hbox->pack_start(*_listButtons.remove, true, true, 0);

	_listButtons.add->signal_clicked().connect(sigc::mem_fun(*this, &CustomStimEditor::onAddStimType));
	_listButtons.remove->signal_clicked().connect(sigc::mem_fun(*this, &CustomStimEditor::onRemoveStimType));

	return *hbox;
}

void CustomStimEditor::entryChanged(Gtk::Entry* entry)
{
	if (entry == _propertyWidgets.nameEntry)
	{
		// Set the caption of the curently selected stim type
		std::string caption = _propertyWidgets.nameEntry->get_text();

		// Pass the call to the helper class
		_stimTypes.setStimTypeCaption(getIdFromSelection(), caption);

		if (_entity != NULL)
		{
			_entity->updateListStores();
		}
	}
}

void CustomStimEditor::update()
{
	_updatesDisabled = true;

	int id = getIdFromSelection();

	if (id > 0)
	{
		_propertyWidgets.vbox->set_sensitive(true);

		StimType stimType = _stimTypes.get(id);
		_propertyWidgets.nameEntry->set_text(stimType.caption);

		_contextMenu.remove->set_sensitive(true);
	}
	else
	{
		_propertyWidgets.vbox->set_sensitive(false);
		_contextMenu.remove->set_sensitive(false);
	}

	_updatesDisabled = false;
}

void CustomStimEditor::selectId(int id)
{
	// Setup the selectionfinder to search for the id
	gtkutil::TreeModel::SelectionFinder finder(id, _stimTypes.getColumns().id.index());

	_customStimStore->foreach_iter(
		sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	if (finder.getIter())
	{
		// Set the active row of the list to the given stim
		_list->get_selection()->select(finder.getIter());
	}
}

void CustomStimEditor::addStimType()
{
	// Add a new stim type with the lowest free custom id
	int id = _stimTypes.getFreeCustomStimId();

	_stimTypes.add(id,
				   string::to_string(id),
				   "CustomStimType",
				   _("Custom Stim"),
				   ICON_CUSTOM_STIM,
				   true);

	selectId(id);
}

int CustomStimEditor::getIdFromSelection()
{
	Gtk::TreeModel::iterator iter = _list->get_selection()->get_selected();

	return (iter) ? (*iter)[_stimTypes.getColumns().id] : -1;
}

void CustomStimEditor::removeStimType()
{
	IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Delete Custom Stim"),
		_("Beware that other entities <i>might</i> still be using this stim type.\n"
		"Do you really want to delete this custom stim?"), ui::IDialog::MESSAGE_ASK);

	if (dialog->run() == IDialog::RESULT_YES)
	{
		_stimTypes.remove(getIdFromSelection());
	}
}

void CustomStimEditor::onAddStimType()
{
	addStimType();
}

void CustomStimEditor::onRemoveStimType()
{
	// Delete the selected stim type from the list
	removeStimType();
}

void CustomStimEditor::onEntryChanged()
{
	if (_updatesDisabled) return; // Callback loop guard

	entryChanged(_propertyWidgets.nameEntry);
}

void CustomStimEditor::onSelectionChange()
{
	update();
}

// Delete context menu items activated
void CustomStimEditor::onContextMenuDelete()
{
	// Delete the selected stim from the list
	removeStimType();
}

// Delete context menu items activated
void CustomStimEditor::onContextMenuAdd()
{
	addStimType();
}

bool CustomStimEditor::onTreeViewButtonRelease(GdkEventButton* ev)
{
	// Single click with RMB (==> open context menu)
	if (ev->button == 3)
	{
		_contextMenu.menu->popup(1, gtk_get_current_event_time());
	}

	return false;
}

} // namespace ui
