#include "AIHeadChooserDialog.h"

#include "i18n.h"
#include "ieclass.h"
#include "ifavourites.h"

#include "eclass.h"

#include <wx/splitter.h>

#include "wxutil/Bitmap.h"
#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Choose AI Head");
}

class ThreadedAIHeadLoader :
    public wxutil::ThreadedResourceTreePopulator
{
private:
    const wxutil::DeclarationTreeView::Columns& _columns;
    std::set<std::string> _favourites;

    wxIcon _headIcon;

public:
    ThreadedAIHeadLoader(const wxutil::DeclarationTreeView::Columns& columns) :
        ThreadedResourceTreePopulator(columns),
        _columns(columns)
    {
        // Get the list of favourites
        _favourites = GlobalFavouritesManager().getFavourites(decl::getTypeName(decl::Type::EntityDef));

        _headIcon.CopyFromBitmap(wxutil::GetLocalBitmap("icon_classname.png"));
    }

    ~ThreadedAIHeadLoader()
    {
        EnsureStopped();
    }

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        GlobalEntityClassManager().forEachEntityClass([&](const IEntityClassPtr& eclass)
        {
            ThrowIfCancellationRequested();

            if (eclass->getAttributeValue("editor_head") != "1") return;

            bool isFavourite = _favourites.count(eclass->getDeclName()) > 0;

            auto row = model->AddItem();

            row[_columns.iconAndName] = wxVariant(wxDataViewIconText(eclass->getDeclName(), _headIcon));
            row[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite);
            row[_columns.fullName] = eclass->getDeclName();
            row[_columns.leafName] = eclass->getDeclName();
            row[_columns.declName] = eclass->getDeclName();
            row[_columns.isFolder] = false;
            row[_columns.isFavourite] = isFavourite;

            row.SendItemAdded();
        });
    }
};

AIHeadChooserDialog::AIHeadChooserDialog() :
    DialogBase(_(WINDOW_TITLE)),
	_headsView(nullptr),
	_description(nullptr)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	auto splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, 
		wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

	GetSizer()->Add(splitter, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

    auto treeViewPanel = new wxPanel(splitter);
    treeViewPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	_headsView = new wxutil::DeclarationTreeView(treeViewPanel, decl::Type::EntityDef, _columns, wxDV_NO_HEADER);
	_headsView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &AIHeadChooserDialog::onHeadSelectionChanged, this);

	// Head Name column
	_headsView->AppendIconTextColumn("", _columns.iconAndName.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Allow searching for the name
    _headsView->AddSearchColumn(_columns.leafName);

    auto treeViewToolbar = new wxutil::ResourceTreeViewToolbar(treeViewPanel, _headsView);
    treeViewPanel->GetSizer()->Add(treeViewToolbar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    treeViewPanel->GetSizer()->Add(_headsView, 1, wxEXPAND);

	FitToScreen(0.7f, 0.6f);

	auto previewPanel = new wxPanel(splitter, wxID_ANY);

	// Set the default rotation to something better suitable for the head models
	_preview.reset(new wxutil::ModelPreview(previewPanel));
	_preview->setDefaultCamDistanceFactor(9.0f);

	_description = new wxTextCtrl(previewPanel, wxID_ANY, "", 
		wxDefaultPosition, wxDefaultSize, wxTE_LEFT | wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
	_description->SetMinClientSize(wxSize(-1, 60));

	previewPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	previewPanel->GetSizer()->Add(_description, 0, wxEXPAND | wxBOTTOM, 6);
	previewPanel->GetSizer()->Add(_preview->getWidget(), 1, wxEXPAND);

	splitter->SplitVertically(treeViewPanel, previewPanel);

	// Set the default size of the window
	splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.3f));

    populateHeadStore();

    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &AIHeadChooserDialog::_onItemActivated, this);
}

void AIHeadChooserDialog::populateHeadStore()
{
    _headsView->Populate(std::make_shared<ThreadedAIHeadLoader>(_columns));
}

void AIHeadChooserDialog::_onItemActivated(wxDataViewEvent& ev)
{
    EndModal(wxID_OK);
}

void AIHeadChooserDialog::setSelectedHead(const std::string& headDef)
{
    _headsView->SetSelectedDeclName(headDef);
}

std::string AIHeadChooserDialog::getSelectedHead()
{
    return _headsView->GetSelectedDeclName();
}

void AIHeadChooserDialog::handleSelectionChanged()
{
	// Prepare to check for a selection
    auto item = _headsView->GetSelection();

    // Add button is enabled if there is a selection and it is not a folder.
    if (item.IsOk())
    {
        // Make the OK button active
		FindWindowById(wxID_OK, this)->Enable(true);
        _description->Enable(true);

        // Set the panel text with the usage information
		wxutil::TreeModel::Row row(item, *_headsView->GetTreeModel());
        _selectedHead = row[_columns.declName];

        // Lookup the IEntityClass instance
        auto ecls = GlobalEntityClassManager().findClass(_selectedHead);

        if (ecls)
        {
            _preview->setModel(ecls->getAttributeValue("model"));
            _preview->setSkin(ecls->getAttributeValue("skin"));

            // Update the usage panel
            _description->SetValue(eclass::getUsage(ecls));
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

} // namespace
