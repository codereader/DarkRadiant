#include "ColourSchemeEditor.h"

#include "iregistry.h"
#include "imainframe.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "i18n.h"

#include "wxutil/dialog/Dialog.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/TreeView.h"
#include "registry/registry.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/clrpicker.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/checkbox.h>

namespace ui
{

namespace
{
    const char* const EDITOR_WINDOW_TITLE = N_("Edit Colour Schemes");

    constexpr const char* RKEY_OVERRIDE_LIGHTCOL = "user/ui/colour/overrideLightColour";
}

ColourSchemeEditor::ColourSchemeEditor() :
	DialogBase(_(EDITOR_WINDOW_TITLE))
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

void ColourSchemeEditor::populateTree()
{
	GlobalColourSchemeManager().foreachScheme(
        [&](const std::string& name, colours::IColourScheme&)
        {
            wxVector<wxVariant> row;
            row.push_back(wxVariant(name));
            _schemeList->AppendItem(row);
        }
    );
}

wxBoxSizer* ColourSchemeEditor::constructListButtons()
{
    wxBoxSizer* buttonBox = new wxBoxSizer(wxHORIZONTAL);

    _deleteButton = new wxButton(this, wxID_DELETE, _("Delete"));
    wxButton* copyButton = new wxButton(this, wxID_COPY, _("Copy"));

    buttonBox->Add(copyButton, 1, wxEXPAND | wxRIGHT, 6);
    buttonBox->Add(_deleteButton, 1, wxEXPAND);

    copyButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { copyScheme(); });
    _deleteButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { deleteScheme(); });

    return buttonBox;
}

void ColourSchemeEditor::addOptionsPanel(wxBoxSizer& vbox)
{
    wxStaticLine* sep = new wxStaticLine(this);
    vbox.Add(sep, 0, wxEXPAND | wxTOP, 6);

    // Override light colour checkbox
    wxCheckBox* overrideLightsCB = new wxCheckBox(
        this, wxID_ANY, _("Override light volume colour")
    );
    overrideLightsCB->SetValue(
        registry::getValue(RKEY_OVERRIDE_LIGHTCOL, false)
    );
    overrideLightsCB->Bind(
        wxEVT_CHECKBOX,
        [this](wxCommandEvent& ev)
        {
            registry::setValue(RKEY_OVERRIDE_LIGHTCOL, ev.IsChecked());
        }
    );

    vbox.Add(overrideLightsCB, 0, wxEXPAND | wxTOP, 6);
}

void ColourSchemeEditor::constructWindow()
{
    wxBoxSizer* mainHBox = new wxBoxSizer(wxHORIZONTAL);

    GetSizer()->Add(mainHBox, 1, wxEXPAND | wxALL, 12);
    GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0,
        wxALIGN_RIGHT | wxLEFT | wxBOTTOM | wxRIGHT, 12);

    // Create the scheme list and the buttons
    wxBoxSizer* leftVBox = new wxBoxSizer(wxVERTICAL);
    mainHBox->Add(leftVBox, 0, wxEXPAND | wxRIGHT, 6);

    _schemeList = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition,
                                     wxDefaultSize, wxDV_NO_HEADER);
    _schemeList->SetMinClientSize(wxSize(256, -1));
    leftVBox->Add(_schemeList, 1, wxEXPAND | wxBOTTOM, 6);

    // Create a text column to show the scheme name
    _schemeList->AppendTextColumn(
        _("Colour scheme"), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE,
        wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE
    );

    // Connect the signal AFTER selecting the active scheme
    _schemeList->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
        wxDataViewEventHandler(ColourSchemeEditor::callbackSelChanged), NULL, this);

    // Treeview buttons
    leftVBox->Add(constructListButtons(), 0, wxEXPAND, 6);

    // Options panel below the copy/delete buttons
    addOptionsPanel(*leftVBox);

    // The Box containing the Colour, pack it into the right half of the hbox
    _colourFrame = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxDOUBLE_BORDER);
    mainHBox->Add(_colourFrame, 1, wxEXPAND);
}

void ColourSchemeEditor::selectActiveScheme()
{
    // Find a row matching the active colour scheme name
    wxString name = GlobalColourSchemeManager().getActiveScheme().getName();
    unsigned r = 0;
    for ( ; r < _schemeList->GetItemCount(); ++r)
    {
        wxString rowName = _schemeList->GetTextValue(r, 0);
        if (rowName == name)
            break;
    }

	_schemeList->SelectRow(r);
	selectionChanged();
}

void ColourSchemeEditor::deleteSchemeFromList()
{
    // Delete the selected row
	int row = _schemeList->GetSelectedRow();
	if (row != wxNOT_FOUND)
		_schemeList->DeleteItem(row);

	// Select the first scheme
    if (_schemeList->GetItemCount() > 0)
        _schemeList->SelectRow(0);
}

std::string ColourSchemeEditor::getSelectedScheme()
{
	int row = _schemeList->GetSelectedRow();
	if (row != wxNOT_FOUND)
		return _schemeList->GetTextValue(row, 0).ToStdString();
    else
        return "";
}

wxSizer* ColourSchemeEditor::constructColourSelector(colours::IColourItem& colour, const std::string& name)
{
	// Get the description of this colour item from the registry
	std::string descriptionPath = std::string("user/ui/colourschemes/descriptions/") + name;
	std::string description = GlobalRegistry().get(descriptionPath);

	// Give gettext a chance to translate the colour description
	description = _(description.c_str());

	// Create a new colour button
	wxColour tempColour;
	Vector3 tempColourVector = colour.getColour();
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
	auto& scheme = GlobalColourSchemeManager().getColourScheme(getSelectedScheme());

	scheme.foreachColour([&](const std::string& name, colours::IColourItem& item)
	{
		wxSizer* colourSelector = constructColourSelector(item, name);
		_colourFrame->GetSizer()->Add(colourSelector, 0);
	});

	_colourFrame->Layout();
	_colourFrame->Fit();
}

void ColourSchemeEditor::updateWindows()
{
    // Force an eclass update for previewing purposes
    // If the colours are reverted later, this will be cleaned up
    // by the Ok/Cancel handling code anyway
    GlobalColourSchemeManager().emitEclassOverrides();

	signal_ColoursChanged().emit();

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
	auto& scheme = GlobalColourSchemeManager().getColourScheme(activeScheme);
	_deleteButton->Enable(!scheme.isReadOnly());

	// Set the active Scheme, so that the views are updated accordingly
	GlobalColourSchemeManager().setActive(activeScheme);

	updateWindows();
}

void ColourSchemeEditor::deleteScheme()
{
	std::string name = getSelectedScheme();
	// Get the selected scheme
	auto& scheme = GlobalColourSchemeManager().getColourScheme(name);

	if (!scheme.isReadOnly())
	{
		// Remove the actual scheme from the ColourSchemeManager
		GlobalColourSchemeManager().deleteScheme(name);

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
	if (GlobalColourSchemeManager().schemeExists(newName))
	{
		wxutil::Messagebox::ShowError(_("A Scheme with that name already exists."), this);
		return;
	}

	// Copy the scheme
	GlobalColourSchemeManager().copyScheme(name, newName);
	GlobalColourSchemeManager().setActive(newName);

	// Add the new list item to the ListStore
    wxVector<wxVariant> rowData;
    rowData.push_back(wxVariant(newName));
    _schemeList->AppendItem(rowData);

	// Highlight the copied scheme
	selectActiveScheme();
}

void ColourSchemeEditor::callbackColorChanged(wxColourPickerEvent& ev, colours::IColourItem& item)
{
	auto* colourPicker = dynamic_cast<wxColourPickerCtrl*>(ev.GetEventObject());
	auto colour = colourPicker->GetColour();

	// Update the colourItem class
	item.getColour().set(
		static_cast<double>(colour.Red()) / 255.0,
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
		GlobalColourSchemeManager().setActive(getSelectedScheme());
		GlobalColourSchemeManager().saveColourSchemes();
	}
	else
	{
		// Restore all the colour settings from the XMLRegistry, changes get lost
		GlobalColourSchemeManager().restoreColourSchemes();
	}

	// Call the update, so all colours are displayed
	updateWindows();

	return returnCode;
}

void ColourSchemeEditor::DisplayDialog(const cmd::ArgumentList& args)
{
	 ColourSchemeEditor* editor = new ColourSchemeEditor;
	 editor->ShowModal(); // enter main loop
	 editor->Destroy();
}

sigc::signal<void>& ColourSchemeEditor::signal_ColoursChanged()
{
	static sigc::signal<void> _signal;
	return _signal;
}

} // namespace ui
