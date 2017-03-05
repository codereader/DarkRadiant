#include "ColourSchemeEditor.h"

#include "ColourSchemeManager.h"
#include "iregistry.h"
#include "imainframe.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "i18n.h"

#include "wxutil/dialog/Dialog.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/TreeView.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/clrpicker.h>
#include <wx/stattext.h>

namespace ui 
{

namespace
{
	const char* const EDITOR_WINDOW_TITLE = N_("Edit Colour Schemes");
}

ColourSchemeEditor::ColourSchemeEditor() :
	DialogBase(_(EDITOR_WINDOW_TITLE)),
	_listStore(new wxutil::TreeModel(_columns, true))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	constructWindow();

	// Load all the list items
  	populateTree();

	// Highlight the currently selected scheme
	selectActiveScheme();
	updateColourSelectors();

	Layout();
	Fit();
	CenterOnParent();
}

/*	Loads all the scheme items into the list
 */
void ColourSchemeEditor::populateTree()
{
	ColourSchemeMap allSchemes = ColourSchemeManager::Instance().getSchemeList();

	for (ColourSchemeMap::iterator scheme = allSchemes.begin();
		 scheme != allSchemes.end(); ++scheme)
	{
		wxutil::TreeModel::Row row = _listStore->AddItem();

		row[_columns.name] = scheme->first;

		row.SendItemAdded();
	}
}

void ColourSchemeEditor::constructWindow()
{
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	GetSizer()->Add(hbox, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, 
		wxALIGN_RIGHT | wxLEFT | wxBOTTOM | wxRIGHT, 12);

	// Create the treeview and the buttons
	wxBoxSizer* treeViewVbox = new wxBoxSizer(wxVERTICAL);
	hbox->Add(treeViewVbox, 0, wxEXPAND | wxRIGHT, 6);

	_treeView = wxutil::TreeView::CreateWithModel(this, _listStore, wxDV_NO_HEADER);
	_treeView->SetMinClientSize(wxSize(200, -1));
	treeViewVbox->Add(_treeView, 1, wxEXPAND | wxBOTTOM, 6);

	// Create a new column and set its parameters
	_treeView->AppendTextColumn(_("Colour"), _columns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Connect the signal AFTER selecting the active scheme
	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
		wxDataViewEventHandler(ColourSchemeEditor::callbackSelChanged), NULL, this);

	// Treeview buttons
	wxBoxSizer* buttonBox = new wxBoxSizer(wxHORIZONTAL);
	treeViewVbox->Add(buttonBox, 0, wxEXPAND, 6);

	_deleteButton = new wxButton(this, wxID_DELETE, _("Delete"));
	wxButton* copyButton = new wxButton(this, wxID_COPY, _("Copy"));

	buttonBox->Add(copyButton, 1, wxEXPAND | wxRIGHT, 6);
	buttonBox->Add(_deleteButton, 1, wxEXPAND);

	copyButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ColourSchemeEditor::callbackCopy), NULL, this);
	_deleteButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ColourSchemeEditor::callbackDelete), NULL, this);

	// The Box containing the Colour, pack it into the right half of the hbox
	_colourFrame = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxDOUBLE_BORDER);
	hbox->Add(_colourFrame, 1, wxEXPAND);
}

void ColourSchemeEditor::selectActiveScheme()
{
	wxDataViewItem found = _listStore->FindString(
		ColourSchemeManager::Instance().getActiveScheme().getName(), _columns.name);

	_treeView->Select(found);
	selectionChanged();
}

void ColourSchemeEditor::deleteSchemeFromList()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (item.IsOk())
	{
		_listStore->RemoveItem(item);
	}

	// Select the first scheme
	wxDataViewItemArray children;

	if (_listStore->GetChildren(_listStore->GetRoot(), children) > 0)
	{
		_treeView->Select(*children.begin());
		selectionChanged();
	}
}

std::string ColourSchemeEditor::getSelectedScheme()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_listStore);
		return row[_columns.name];
	}

	return "";
}

wxSizer* ColourSchemeEditor::constructColourSelector(ColourItem& colour, const std::string& name)
{
	// Get the description of this colour item from the registry
	std::string descriptionPath = std::string("user/ui/colourschemes/descriptions/") + name;
	std::string description = GlobalRegistry().get(descriptionPath);

	// Give gettext a chance to translate the colour description
	description = _(description.c_str());

	// Create a new colour button
	wxColour tempColour;
	Vector3 tempColourVector = colour;
	tempColour.Set(tempColourVector[0] * 255, tempColourVector[1] * 255, tempColourVector[2] * 255);

	// Create the colour button
	wxColourPickerCtrl* button = new wxColourPickerCtrl(_colourFrame, wxID_ANY);
	button->SetColour(tempColour);

	button->Bind(wxEVT_COLOURPICKER_CHANGED, [&] (wxColourPickerEvent& ev)
	{
		callbackColorChanged(ev, colour);
	});
	
	// Create the description label
	wxStaticText* label = new wxStaticText(_colourFrame, wxID_ANY, description);

	// Create a new horizontal divider
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	hbox->Add(button, 0);
	hbox->Add(label, 0, wxLEFT, 12);

	return hbox;
}

void ColourSchemeEditor::updateColourSelectors()
{
	// Destroy the current _colourBox instance
	if (_colourFrame->GetSizer() != NULL)
	{
		_colourFrame->GetSizer()->DeleteWindows();
	}

	_colourFrame->SetSizer(new wxGridSizer(3, 12, 12));

	// Get the selected scheme
	ColourScheme& scheme = ColourSchemeManager::Instance().getScheme(getSelectedScheme());

	// Retrieve the list with all the ColourItems of this scheme
	ColourItemMap& colourMap = scheme.getColourMap();

	// Cycle through all the ColourItems and save them into the registry
	for (ColourItemMap::iterator it = colourMap.begin(); it != colourMap.end(); ++it)
	{
		wxSizer* colourSelector = constructColourSelector(it->second, it->first);
		_colourFrame->GetSizer()->Add(colourSelector, 0);
	}

	_colourFrame->Layout();
	_colourFrame->Fit();
}

void ColourSchemeEditor::updateWindows()
{
	// Call the update, so all colours can be previewed
	GlobalMainFrame().updateAllWindows();
	SceneChangeNotify();
}

void ColourSchemeEditor::selectionChanged()
{
	std::string activeScheme = getSelectedScheme();

	// Update the colour selectors to reflect the newly selected scheme
	updateColourSelectors();

	// Check, if the currently selected scheme is read-only
	ColourScheme& scheme = ColourSchemeManager::Instance().getScheme(activeScheme);
	_deleteButton->Enable(!scheme.isReadOnly());

	// Set the active Scheme, so that the views are updated accordingly
	ColourSchemeManager::Instance().setActive(activeScheme);

	updateWindows();
}

void ColourSchemeEditor::deleteScheme()
{
	std::string name = getSelectedScheme();
	// Get the selected scheme
	ColourScheme& scheme = ColourSchemeManager::Instance().getScheme(name);

	if (!scheme.isReadOnly())
	{
		// Remove the actual scheme from the ColourSchemeManager
		ColourSchemeManager::Instance().deleteScheme(name);

		// Remove the selected item from the GtkListStore
		deleteSchemeFromList();
	}
}

std::string ColourSchemeEditor::inputDialog(const std::string& title, const std::string& label)
{
	wxutil::Dialog dialog(title, this);

	IDialog::Handle entryHandle = dialog.addEntryBox(label);

	if (dialog.run() == IDialog::RESULT_OK)
	{
		return dialog.getElementValue(entryHandle);
	}
	else
	{
		return "";
	}
}

void ColourSchemeEditor::copyScheme()
{
	std::string name = getSelectedScheme();
	std::string newName = inputDialog(_("Copy Colour Scheme"), _("Enter a name for the new scheme:"));

	if (newName.empty())
	{
		return; // empty name
	}

	// greebo: Check if the new name is already existing
	if (ColourSchemeManager::Instance().schemeExists(newName))
	{
		wxutil::Messagebox::ShowError(_("A Scheme with that name already exists."), this);
		return;
	}

	// Copy the scheme
	ColourSchemeManager::Instance().copyScheme(name, newName);
	ColourSchemeManager::Instance().setActive(newName);

	// Add the new list item to the ListStore
	wxutil::TreeModel::Row row = _listStore->AddItem();
	row[_columns.name] = newName;
	row.SendItemAdded();

	// Highlight the copied scheme
	selectActiveScheme();
}

void ColourSchemeEditor::callbackCopy(wxCommandEvent& ev)
{
	copyScheme();
}

void ColourSchemeEditor::callbackDelete(wxCommandEvent& ev)
{
	deleteScheme();
}

void ColourSchemeEditor::callbackColorChanged(wxColourPickerEvent& ev, ColourItem& item)
{
	wxColourPickerCtrl* colourPicker = dynamic_cast<wxColourPickerCtrl*>(ev.GetEventObject());
	wxColour colour = colourPicker->GetColour();

	// Update the colourItem class
	item.set(static_cast<double>(colour.Red()) / 255.0, 
		     static_cast<double>(colour.Green()) / 255.0,
			 static_cast<double>(colour.Blue()) / 255.0);

	// Call the update, so all colours can be previewed
	updateWindows();
}

// This is called when the colourscheme selection is changed
void ColourSchemeEditor::callbackSelChanged(wxDataViewEvent& ev)
{
	selectionChanged();
}

int ColourSchemeEditor::ShowModal()
{
	int returnCode = DialogBase::ShowModal();
	
	if (returnCode == wxID_OK)
	{
		ColourSchemeManager::Instance().setActive(getSelectedScheme());
		ColourSchemeManager::Instance().saveColourSchemes();
	}
	else
	{
		// Restore all the colour settings from the XMLRegistry, changes get lost
		ColourSchemeManager::Instance().restoreColourSchemes();

		// Call the update, so all restored colours are displayed
		updateWindows();
	}

	return returnCode;
}

void ColourSchemeEditor::DisplayDialog(const cmd::ArgumentList& args)
{
	 ColourSchemeEditor* editor = new ColourSchemeEditor;
	 editor->ShowModal(); // enter main loop
	 editor->Destroy();
}

} // namespace ui
