#include "ClassEditor.h"

#include "string/convert.h"
#include <iostream>
#include "i18n.h"

#include <wx/bmpcbox.h>
#include <wx/sizer.h>

namespace ui
{

namespace
{
	const int TREE_VIEW_WIDTH = 320;
	const int TREE_VIEW_HEIGHT = 160;
}

ClassEditor::ClassEditor(wxWindow* parent, StimTypes& stimTypes) :
	wxPanel(parent, wxID_ANY),
	_stimTypes(stimTypes),
	_updatesDisabled(false)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 6);

	_list = wxutil::TreeView::Create(parent);
	_list->SetMinClientSize(wxSize(TREE_VIEW_WIDTH, TREE_VIEW_HEIGHT));
	vbox->Add(_list, 1, wxEXPAND | wxBOTTOM, 6);

	// Connect the signals to the callbacks
	_list->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
        wxDataViewEventHandler(ClassEditor::onSRSelectionChange), NULL, this);
	_list->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(ClassEditor::onTreeViewKeyPress), NULL, this);
	_list->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, 
		wxDataViewEventHandler(ClassEditor::onContextMenu), NULL, this);

	// Add the columns to the treeview
	// ID number
	_list->AppendTextColumn("#", SREntity::getColumns().index.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
	
	// The S/R icon
	_list->AppendBitmapColumn(_("S/R"), SREntity::getColumns().srClass.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	// The Type
	_list->AppendIconTextColumn(_("Type"), SREntity::getColumns().icon.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
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

wxBitmapComboBox* ClassEditor::createStimTypeSelector(wxWindow* parent)
{
	wxBitmapComboBox* combo = new wxBitmapComboBox(parent, 
		wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);

	_stimTypes.populateBitmapComboBox(combo);
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

void ClassEditor::onSRSelectionChange(wxDataViewEvent& ev)
{
	selectionChanged();
}

void ClassEditor::onTreeViewKeyPress(wxKeyEvent& ev)
{
	if (ev.GetKeyCode() == WXK_DELETE)
	{
		removeSR();
		return;
	}

	// Propagate further
	ev.Skip();
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

void ClassEditor::onContextMenu(wxDataViewEvent& ev)
{
	// wxTODO _popupMenu->show(_treeView);
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
