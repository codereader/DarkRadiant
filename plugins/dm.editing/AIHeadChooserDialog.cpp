#include "AIHeadChooserDialog.h"

#include "i18n.h"
#include "ieclass.h"

#include "eclass.h"

#include <wx/splitter.h>

#include "wxutil/Bitmap.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "ThreadedEntityDefPopulator.h"

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Choose AI Head");
}

class ThreadedAIHeadLoader :
    public ThreadedEntityDefPopulator
{
public:
    ThreadedAIHeadLoader(const wxutil::DeclarationTreeView::Columns& columns) :
        ThreadedEntityDefPopulator(columns, "icon_classname.png")
    {}

protected:
    bool ClassShouldBeListed(const IEntityClassPtr& eclass) override
    {
        return eclass->getAttributeValue("editor_head") == "1";
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
	_preview->setDefaultCamDistanceFactor(5.0f);

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
    _selectedHead = _headsView->GetSelectedDeclName();

    // Update sensitivity
    FindWindowById(wxID_OK, this)->Enable(!_selectedHead.empty());
    _description->Enable(!_selectedHead.empty());

    if (!_selectedHead.empty())
    {
        // Lookup the IEntityClass instance
        if (auto ecls = GlobalEntityClassManager().findClass(_selectedHead); ecls)
        {
            _preview->setModel(ecls->getAttributeValue("model"));
            _preview->setSkin(ecls->getAttributeValue("skin"));

            // Update the usage panel
            _description->SetValue(eclass::getUsage(ecls));
        }
    }
    else
    {
        _preview->setModel("");
    }
}

void AIHeadChooserDialog::onHeadSelectionChanged(wxDataViewEvent& ev)
{
    handleSelectionChanged();
}

} // namespace
