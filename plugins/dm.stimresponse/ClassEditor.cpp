#include "ClassEditor.h"

#include "gtkutil/TreeModel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "string/convert.h"
#include <iostream>
#include "i18n.h"

#include <gtkmm/treeview.h>
#include <gtkmm/entry.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/combobox.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/image.h>

namespace ui
{

	namespace
	{
		const int TREE_VIEW_WIDTH = 320;
		const int TREE_VIEW_HEIGHT = 160;
	}

ClassEditor::ClassEditor(StimTypes& stimTypes) :
	Gtk::VBox(false, 6),
	_stimTypes(stimTypes),
	_updatesDisabled(false)
{
	set_border_width(6);

	_list = Gtk::manage(new Gtk::TreeView);
	_list->set_size_request(TREE_VIEW_WIDTH, TREE_VIEW_HEIGHT);

	// Connect the signals to the callbacks
	_list->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &ClassEditor::onSRSelectionChange));
	_list->signal_key_press_event().connect(sigc::mem_fun(*this, &ClassEditor::onTreeViewKeyPress), false);
	_list->signal_button_release_event().connect(
		sigc::bind(sigc::mem_fun(*this, &ClassEditor::onTreeViewButtonRelease), _list));

	// Add the columns to the treeview
	// ID number
	Gtk::TreeViewColumn* numCol = Gtk::manage(new Gtk::TreeViewColumn("#"));
	Gtk::CellRendererText* numRenderer = Gtk::manage(new Gtk::CellRendererText);

	numCol->pack_start(*numRenderer, false);
	numCol->add_attribute(numRenderer->property_text(), SREntity::getColumns().index);
	numCol->add_attribute(numRenderer->property_foreground(), SREntity::getColumns().colour);

	_list->append_column(*numCol);

	// The S/R icon
	Gtk::TreeViewColumn* classCol = Gtk::manage(new Gtk::TreeViewColumn(_("S/R")));

	Gtk::CellRendererPixbuf* pixbufRenderer = Gtk::manage(new Gtk::CellRendererPixbuf);

	classCol->pack_start(*pixbufRenderer, false);
	classCol->add_attribute(pixbufRenderer->property_pixbuf(), SREntity::getColumns().srClass);

	_list->append_column(*classCol);

	// The Type
	Gtk::TreeViewColumn* typeCol = Gtk::manage(new Gtk::TreeViewColumn(_("Type")));

	Gtk::CellRendererPixbuf* typeIconRenderer = Gtk::manage(new Gtk::CellRendererPixbuf);

	typeCol->pack_start(*typeIconRenderer, false);
	typeCol->add_attribute(typeIconRenderer->property_pixbuf(), SREntity::getColumns().icon);

	Gtk::CellRendererText* typeTextRenderer = Gtk::manage(new Gtk::CellRendererText);

	typeCol->pack_start(*typeTextRenderer, false);
	typeCol->add_attribute(typeTextRenderer->property_text(), SREntity::getColumns().caption);
	typeCol->add_attribute(typeTextRenderer->property_foreground(), SREntity::getColumns().colour);

	_list->append_column(*typeCol);
}

void ClassEditor::setEntity(const SREntityPtr& entity)
{
	_entity = entity;
}

int ClassEditor::getIdFromSelection()
{
	Gtk::TreeModel::iterator iter = _list->get_selection()->get_selected();

	if (iter && _entity != NULL)
	{
		return (*iter)[SREntity::getColumns().id];
	}
	else
	{
		return -1;
	}
}

void ClassEditor::setProperty(const std::string& key, const std::string& value)
{
	int id = getIdFromSelection();

	if (id > 0)
	{
		// Don't edit inherited stims/responses
		_entity->setProperty(id, key, value);
	}

	// Call the method of the child class to update the widgets
	update();
}

void ClassEditor::entryChanged(Gtk::Entry* entry)
{
	// Try to find the key this entry widget is associated to
	EntryMap::iterator found = _entryWidgets.find(entry);

	if (found != _entryWidgets.end())
	{
		std::string entryText = entry->get_text();

		if (!entryText.empty())
		{
			setProperty(found->second, entryText);
		}
	}
}

void ClassEditor::spinButtonChanged(Gtk::SpinButton* spinButton)
{
	// Try to find the key this spinbutton widget is associated to
	SpinButtonMap::iterator found = _spinWidgets.find(spinButton);

	if (found != _spinWidgets.end())
	{
		std::string valueText = string::to_string(spinButton->get_value());

		if (!valueText.empty())
		{
			setProperty(found->second, valueText);
		}
	}
}

ClassEditor::TypeSelectorWidgets ClassEditor::createStimTypeSelector()
{
	TypeSelectorWidgets widgets;

	// Type Selector
	widgets.hbox = Gtk::manage(new Gtk::HBox(false, 0));

	widgets.label = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Type:")));

	// Cast the helper class onto a ListStore and create a new treeview
	widgets.list = Gtk::manage(new Gtk::ComboBox(_stimTypes.getListStore()));
	widgets.list->set_size_request(-1, -1);

	// Add the cellrenderer for the name
	Gtk::CellRendererText* nameRenderer = Gtk::manage(new Gtk::CellRendererText);
	Gtk::CellRendererPixbuf* iconRenderer = Gtk::manage(new Gtk::CellRendererPixbuf);

	widgets.list->pack_start(*iconRenderer, false);
	widgets.list->pack_start(*nameRenderer, true);

	widgets.list->add_attribute(iconRenderer->property_pixbuf(), _stimTypes.getColumns().icon);
	widgets.list->add_attribute(nameRenderer->property_text(), _stimTypes.getColumns().captionPlusID);
	iconRenderer->set_fixed_size(26, -1);

	widgets.hbox->pack_start(*widgets.label, false, false, 0);
	widgets.hbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*widgets.list, 12, 1.0f)),
		true, true,	0
	);

	// Set the combo box to use two-column
	widgets.list->set_wrap_width(2);
	widgets.list->set_active(0);

	return widgets;
}

Gtk::Widget& ClassEditor::createListButtons()
{
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	// Create the type selector and pack it
	_addType = createStimTypeSelector();
	hbox->pack_start(*_addType.hbox, true, true, 0);

	_listButtons.add = Gtk::manage(new Gtk::Button(_("Add")));
	_listButtons.add->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::ADD, Gtk::ICON_SIZE_BUTTON)));

	_listButtons.remove = Gtk::manage(new Gtk::Button(_("Remove")));
	_listButtons.remove->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON)));

	hbox->pack_start(*_listButtons.add, false, false, 0);
	hbox->pack_start(*_listButtons.remove, false, false, 0);

	_addType.list->signal_changed().connect(sigc::mem_fun(*this, &ClassEditor::onAddTypeSelect));
	_listButtons.add->signal_clicked().connect(sigc::mem_fun(*this, &ClassEditor::onAddSR));
	_listButtons.remove->signal_clicked().connect(sigc::mem_fun(*this, &ClassEditor::onRemoveSR));

	return *hbox;
}

void ClassEditor::removeSR()
{
	// Get the selected stim ID
	int id = getIdFromSelection();

	if (id > 0)
	{
		_entity->remove(id);
	}
}

void ClassEditor::selectId(int id)
{
	// Setup the selectionfinder to search for the id
	gtkutil::TreeModel::SelectionFinder finder(id, SREntity::getColumns().id.index());

	_list->get_model()->foreach_iter(
		sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	if (finder.getIter())
	{
		// Set the active row of the list to the given effect
		_list->get_selection()->select(finder.getIter());
	}
}

void ClassEditor::duplicateStimResponse()
{
	int id = getIdFromSelection();

	if (id > 0)
	{
		int newId = _entity->duplicate(id);
		// Select the newly created stim
		selectId(newId);
	}

	// Call the method of the child class to update the widgets
	update();
}

// Static callbacks
void ClassEditor::onSRSelectionChange()
{
	selectionChanged();
}

bool ClassEditor::onTreeViewKeyPress(GdkEventKey* ev)
{
	if (ev->keyval == GDK_Delete)
	{
		removeSR();

		// Catch this keyevent, don't propagate
		return true;
	}

	// Propagate further
	return false;
}

bool ClassEditor::onTreeViewButtonRelease(GdkEventButton* ev, Gtk::TreeView* view)
{
	// Single click with RMB (==> open context menu)
	if (ev->button == 3)
	{
		openContextMenu(view);
	}

	return false;
}

void ClassEditor::onSpinButtonChanged(Gtk::SpinButton* spinButton)
{
	if (_updatesDisabled) return; // Callback loop guard

	spinButtonChanged(spinButton);
}

void ClassEditor::connectSpinButton(Gtk::SpinButton* spinButton, const std::string& key)
{
	// Associate the spin button with a specific entity key, if not empty
	if (!key.empty())
	{
		_spinWidgets[spinButton] = key;
	}

	// Connect the callback and bind the spinbutton pointer as first argument
	spinButton->signal_value_changed().connect(
		sigc::bind(sigc::mem_fun(*this, &ClassEditor::onSpinButtonChanged), spinButton));
}

void ClassEditor::onEntryChanged(Gtk::Entry* entry)
{
	if (_updatesDisabled) return; // Callback loop guard

	entryChanged(entry);
}

void ClassEditor::connectEntry(Gtk::Entry* entry, const std::string& key)
{
	// Associate the entry with a specific entity key
	_entryWidgets[entry] = key;

	// Connect the callback and bind the entry pointer as first argument
	entry->signal_changed().connect(
		sigc::bind(sigc::mem_fun(*this, &ClassEditor::onEntryChanged), entry));
}

void ClassEditor::onCheckboxToggle(Gtk::CheckButton* toggleButton)
{
	if (_updatesDisabled) return; // Callback loop guard

	checkBoxToggled(toggleButton);
}

void ClassEditor::connectCheckButton(Gtk::CheckButton* checkButton)
{
	// Bind the checkbutton pointer to the callback, it is needed in onCheckboxToggle
	checkButton->signal_toggled().connect(
		sigc::bind(sigc::mem_fun(*this, &ClassEditor::onCheckboxToggle), checkButton));
}

std::string ClassEditor::getStimTypeIdFromSelector(Gtk::ComboBox* widget)
{
	Gtk::TreeModel::iterator iter = widget->get_active();

	if (iter)
	{
		// Load the stim name (e.g. "STIM_FIRE") directly from the liststore
		return Glib::ustring((*iter)[_stimTypes.getColumns().name]);
	}

	return "";
}

void ClassEditor::onStimTypeSelect()
{
	if (_updatesDisabled || _type.list == NULL) return; // Callback loop guard

	std::string name = getStimTypeIdFromSelector(_type.list);

	if (!name.empty())
	{
		// Write it to the entity
		setProperty("type", name);
	}
}

void ClassEditor::onAddTypeSelect()
{
	if (_updatesDisabled || _addType.list == NULL) return; // Callback loop guard

	std::string name = getStimTypeIdFromSelector(_addType.list);

	if (!name.empty())
	{
		addSR();
	}
}

// "Disable" context menu item
void ClassEditor::onContextMenuDisable()
{
	setProperty("state", "0");
}

// "Enable" context menu item
void ClassEditor::onContextMenuEnable()
{
	setProperty("state", "1");
}

void ClassEditor::onContextMenuDuplicate()
{
	duplicateStimResponse();
}

void ClassEditor::onAddSR()
{
	// Add a S/R
	addSR();
}

void ClassEditor::onRemoveSR()
{
	// Delete the selected S/R from the list
	removeSR();
}

} // namespace ui
