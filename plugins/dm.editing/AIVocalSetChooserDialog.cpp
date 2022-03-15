#include "AIVocalSetChooserDialog.h"

#include "i18n.h"
#include "ui/imainframe.h"
#include "ieclass.h"
#include "isound.h"

#include "eclass.h"

#include <wx/stattext.h>
#include <wx/sizer.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Choose AI Vocal Set");
}

AIVocalSetChooserDialog::AIVocalSetChooserDialog() :
	DialogBase(_(WINDOW_TITLE)),
	_setStore(new wxutil::TreeModel(_columns, true)),
	_preview(NULL)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	if (module::GlobalModuleRegistry().moduleExists(MODULE_SOUNDMANAGER))
	{
		_preview = new AIVocalSetPreview(this);
	}

	_setView = wxutil::TreeView::CreateWithModel(this, _setStore.get(), wxDV_NO_HEADER);
	_setView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
		wxDataViewEventHandler(AIVocalSetChooserDialog::onSetSelectionChanged), NULL, this);

	// Head Name column
	_setView->AppendTextColumn("", _columns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Allow searching for the name
	_setView->AddSearchColumn(_columns.name);

	wxBoxSizer* vbox1 = new wxBoxSizer(wxVERTICAL);

	wxStaticText* label1 = new wxStaticText(this, wxID_ANY, _("Available Sets"));
	label1->SetFont(label1->GetFont().Bold());

	vbox1->Add(label1, 0, wxBOTTOM, 6);
	vbox1->Add(_setView, 1, wxEXPAND);

	// Right: the description
	wxBoxSizer* vbox2 = new wxBoxSizer(wxVERTICAL);

	wxStaticText* label2 = new wxStaticText(this, wxID_ANY, _("Description"));
	label2->SetFont(label2->GetFont().Bold());

	_description = new wxTextCtrl(this, wxID_ANY, "", 
		wxDefaultPosition, wxDefaultSize, wxTE_LEFT | wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
	_description->SetMinClientSize(wxSize(-1, 60));

	vbox2->Add(label2, 0, wxBOTTOM, 6);
	vbox2->Add(_description, 1, wxEXPAND | wxBOTTOM, 6);

	// Right: the preview control panel
	if (_preview != NULL)
	{
		vbox2->Add(_preview, 0, wxEXPAND);
	}

	// dialog hbox, left is the treeview, right is the preview panel
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	hbox->Add(vbox1, 3, wxEXPAND | wxRIGHT, 6);
	hbox->Add(vbox2, 1, wxEXPAND | wxRIGHT, 6);

	GetSizer()->Add(hbox, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

	FitToScreen(0.7f, 0.6f);

	// Check if the liststore is populated
	findAvailableSets();

	// Load the found sets into the GtkListStore
	populateSetStore();

    Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &AIVocalSetChooserDialog::_onItemActivated, this );
}

void AIVocalSetChooserDialog::_onItemActivated( wxDataViewEvent& ev ) {
    EndModal( wxID_OK );
}

void AIVocalSetChooserDialog::setSelectedVocalSet(const std::string& setName)
{
	_selectedSet = setName;

	if (_selectedSet.empty())
	{
		_setView->UnselectAll();
		return;
	}

	wxDataViewItem found = _setStore->FindString(setName, _columns.name);

	// Lookup the model path in the treemodel
	if (found.IsOk())
    {
        _setView->Select(found);
		_setView->EnsureVisible(found);
		handleSetSelectionChanged();
	}
}

std::string AIVocalSetChooserDialog::getSelectedVocalSet()
{
	return _selectedSet;
}

void AIVocalSetChooserDialog::handleSetSelectionChanged()
{
	// Prepare to check for a selection
	wxDataViewItem item = _setView->GetSelection();

    // Add button is enabled if there is a selection
    if (item.IsOk())
	{
		// Make the OK button active
		FindWindowById(wxID_OK, this)->Enable(true);
		_description->Enable(true);

		// Set the panel text with the usage information
		wxutil::TreeModel::Row row(item, *_setStore);
		_selectedSet = row[_columns.name];

		// Lookup the IEntityClass instance
		IEntityClassPtr ecls = GlobalEntityClassManager().findClass(_selectedSet);

		if (ecls)
		{
			// Update the preview pane
			if (_preview != NULL)
			{
				_preview->setVocalSetEclass(ecls);
			}

			// Update the usage panel
			_description->SetValue(eclass::getUsage(*ecls));
		}
	}
	else
	{
		_selectedSet = "";

		if (_preview != NULL)
		{
			_preview->setVocalSetEclass(IEntityClassPtr());
		}

		FindWindowById(wxID_OK, this)->Enable(false);
		_description->Enable(false);
	}
}

void AIVocalSetChooserDialog::onSetSelectionChanged(wxDataViewEvent& ev)
{
	handleSetSelectionChanged();
}

void AIVocalSetChooserDialog::populateSetStore()
{
	// Clear the head list to be safe
	_setStore->Clear();

	for (SetList::const_iterator i = _availableSets.begin(); i != _availableSets.end(); ++i)
	{
		// Add the entity to the list
		wxutil::TreeModel::Row row = _setStore->AddItem();

		row[_columns.name] = *i;

		row.SendItemAdded();
	}
}

namespace
{

class VocalSetEClassFinder :
	public EntityClassVisitor
{
	AIVocalSetChooserDialog::SetList& _list;

public:
	VocalSetEClassFinder(AIVocalSetChooserDialog::SetList& list) :
		_list(list)
	{}

	void visit(const IEntityClassPtr& eclass)
	{
		if (eclass->getAttributeValue("editor_vocal_set") == "1")
		{
			_list.insert(eclass->getName());
		}
	}
};

} // namespace

void AIVocalSetChooserDialog::findAvailableSets()
{
	if (!_availableSets.empty())
	{
		return;
	}

	// Instantiate a finder class and traverse all eclasses
	VocalSetEClassFinder visitor(_availableSets);
	GlobalEntityClassManager().forEachEntityClass(visitor);
}

// Init static class member
AIVocalSetChooserDialog::SetList AIVocalSetChooserDialog::_availableSets;

} // namespace ui
