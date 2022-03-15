#include "AIHeadChooserDialog.h"

#include "i18n.h"
#include "ui/imainframe.h"
#include "ieclass.h"

#include "eclass.h"

#include <wx/splitter.h>
#include <wx/button.h>

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Choose AI Head");
}

AIHeadChooserDialog::AIHeadChooserDialog() :
    DialogBase(_(WINDOW_TITLE)),
    _headStore(new wxutil::TreeModel(_columns, true)),
	_headsView(NULL),
	_description(NULL)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, 
		wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

	GetSizer()->Add(splitter, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

	_headsView = wxutil::TreeView::CreateWithModel(splitter, _headStore.get(), wxDV_NO_HEADER);
	_headsView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
        wxDataViewEventHandler(AIHeadChooserDialog::onHeadSelectionChanged), NULL, this);

	// Head Name column
	_headsView->AppendTextColumn("", _columns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Allow searching for the name
    _headsView->AddSearchColumn(_columns.name);

	FitToScreen(0.7f, 0.6f);

	wxPanel* previewPanel = new wxPanel(splitter, wxID_ANY);

	// Set the default rotation to something better suitable for the head models
	_preview.reset(new wxutil::ModelPreview(previewPanel));
	_preview->setDefaultCamDistanceFactor(9.0f);

	_description = new wxTextCtrl(previewPanel, wxID_ANY, "", 
		wxDefaultPosition, wxDefaultSize, wxTE_LEFT | wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
	_description->SetMinClientSize(wxSize(-1, 60));

	previewPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	previewPanel->GetSizer()->Add(_description, 0, wxEXPAND | wxBOTTOM, 6);
	previewPanel->GetSizer()->Add(_preview->getWidget(), 1, wxEXPAND);

	splitter->SplitVertically(_headsView, previewPanel);

	// Set the default size of the window
	splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.3f));

    // Check if the liststore is populated
    findAvailableHeads();

    // Load the found heads into the GtkListStore
    populateHeadStore();

    Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &AIHeadChooserDialog::_onItemActivated, this );
}

void AIHeadChooserDialog::_onItemActivated( wxDataViewEvent& ev ) {
    EndModal( wxID_OK );
}

void AIHeadChooserDialog::setSelectedHead(const std::string& headDef)
{
    _selectedHead = headDef;

    if (_selectedHead.empty())
    {
        _headsView->UnselectAll();
        return;
    }

	wxDataViewItem found = _headStore->FindString(headDef, _columns.name);

    // Lookup the model path in the treemodel
	if (found.IsOk())
    {
        _headsView->Select(found);
		_headsView->EnsureVisible(found);
		handleSelectionChanged();
    }
}

std::string AIHeadChooserDialog::getSelectedHead()
{
    return _selectedHead;
}

void AIHeadChooserDialog::handleSelectionChanged()
{
	// Prepare to check for a selection
    wxDataViewItem item = _headsView->GetSelection();

    // Add button is enabled if there is a selection and it is not a folder.
    if (item.IsOk())
    {
        // Make the OK button active
		FindWindowById(wxID_OK, this)->Enable(true);
        _description->Enable(true);

        // Set the panel text with the usage information
		wxutil::TreeModel::Row row(item, *_headStore);
        _selectedHead = row[_columns.name];

        // Lookup the IEntityClass instance
        IEntityClassPtr ecls = GlobalEntityClassManager().findClass(_selectedHead);

        if (ecls)
        {
            _preview->setModel(ecls->getAttributeValue("model"));
            _preview->setSkin(ecls->getAttributeValue("skin"));

            // Update the usage panel
            _description->SetValue(eclass::getUsage(*ecls));
        }
    }
    else
    {
        _selectedHead = "";
        _preview->setModel("");

		FindWindowById(wxID_OK, this)->Enable(false);
        _description->Enable(false);
    }
}

void AIHeadChooserDialog::onHeadSelectionChanged(wxDataViewEvent& ev)
{
    handleSelectionChanged();
}

void AIHeadChooserDialog::populateHeadStore()
{
    // Clear the head list to be safe
    _headStore->Clear();

    for (HeadList::const_iterator i = _availableHeads.begin(); i != _availableHeads.end(); ++i)
    {
        // Add the entity to the list
        wxutil::TreeModel::Row row = _headStore->AddItem();

        row[_columns.name] = *i;

		row.SendItemAdded();
    }
}

namespace
{

class HeadEClassFinder :
    public EntityClassVisitor
{
    AIHeadChooserDialog::HeadList& _list;

public:
    HeadEClassFinder(AIHeadChooserDialog::HeadList& list) :
        _list(list)
    {}

    void visit(const IEntityClassPtr& eclass)
    {
        if (eclass->getAttributeValue("editor_head") == "1")
        {
            _list.insert(eclass->getName());
        }
    }
};

} // namespace

void AIHeadChooserDialog::findAvailableHeads()
{
    if (!_availableHeads.empty())
    {
        return;
    }

    // Instantiate a finder class and traverse all eclasses
    HeadEClassFinder visitor(_availableHeads);
    GlobalEntityClassManager().forEachEntityClass(visitor);
}

// Init static class member
AIHeadChooserDialog::HeadList AIHeadChooserDialog::_availableHeads;

} // namespace ui
