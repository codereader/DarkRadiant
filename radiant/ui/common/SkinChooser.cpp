#include "SkinChooser.h"

#include "i18n.h"
#include "modelskin.h"

#include <wx/sizer.h>
#include <wx/splitter.h>

#include "ifavourites.h"
#include "debugging/ScopedDebugTimer.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/dataview/VFSTreePopulator.h"

namespace ui
{

namespace
{
	constexpr const char* SKIN_ICON = "skin16.png";

	constexpr const char* const WINDOW_TITLE = N_("Choose Skin");

    /**
     * Visitor class to retrieve skin names and add them to folders.
     */
    class ThreadedSkinLoader final :
        public wxutil::ThreadedDeclarationTreePopulator
    {
    private:
        const wxutil::DeclarationTreeView::Columns& _columns;

        wxDataViewItem& _allSkinsItem;
        wxDataViewItem& _matchingSkinsItem;

        std::string _model;

    public:
        ThreadedSkinLoader(const wxutil::DeclarationTreeView::Columns& columns, const std::string& model, 
            wxDataViewItem& allSkinsItem, wxDataViewItem& matchingSkinsItem) :
            ThreadedDeclarationTreePopulator(decl::Type::Skin, columns, SKIN_ICON),
            _columns(columns),
            _allSkinsItem(allSkinsItem),
            _matchingSkinsItem(matchingSkinsItem),
            _model(model)
        {}

        ~ThreadedSkinLoader()
        {
            EnsureStopped();
        }

    protected:
        void PopulateModel(const wxutil::TreeModel::Ptr& model) override
        {
            ScopedDebugTimer timer("ThreadedSkinLoader::run()");

            wxutil::VFSTreePopulator populator(model);

            auto matchingSkins = _("Matching skins");
            auto allSkins = _("All skins");

            // Add the matching skins node in any case, even if it ends up empty
            populator.addPath(matchingSkins, [&](wxutil::TreeModel::Row& row,
                const std::string& path, const std::string& leafName, bool isFolder)
            {
                AssignValuesToRow(row, path, path, leafName, true);
            });;

            // Get the skins for the associated model, and add them as matching skins
            const auto& matchList = GlobalModelSkinCache().getSkinsForModel(_model);

            for (const auto& skin : matchList)
            {
                populator.addPath(matchingSkins + "/" + skin, [&](wxutil::TreeModel::Row& row,
                    const std::string& path, const std::string& leafName, bool isFolder)
                {
                    AssignValuesToRow(row, path, isFolder ? path : skin, leafName, isFolder);
                });;
            }

            // Get the list of skins for the model
            const auto& skins = GlobalModelSkinCache().getAllSkins();

            for (const auto& skin : skins)
            {
                populator.addPath(allSkins + "/" + skin, [&](wxutil::TreeModel::Row& row,
                    const std::string& path, const std::string& leafName, bool isFolder)
                {
                    AssignValuesToRow(row, path, isFolder ? path : skin, leafName, isFolder);
                });
            }

            _matchingSkinsItem = model->FindString(matchingSkins, _columns.fullName);
            _allSkinsItem = model->FindString(allSkins, _columns.fullName);
        }

        void SortModel(const wxutil::TreeModel::Ptr& model) override
        {
            model->SortModelFoldersFirst(_columns.leafName, _columns.isFolder);
        }
    };
}

SkinChooser::SkinChooser() :
	DialogBase(_(WINDOW_TITLE)),
	_treeView(nullptr),
    _treeViewToolbar(nullptr),
    _materialsList(nullptr),
    _fileInfo(nullptr)
{
    populateWindow();
}

void SkinChooser::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition,
                                                      wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

    auto leftPanel = new wxPanel(splitter, wxID_ANY);
    leftPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	// Create the tree view
    _treeView = new wxutil::DeclarationTreeView(leftPanel, decl::Type::Skin, _columns, wxDV_NO_HEADER);
    _treeView->SetSize(300, -1);
	
	// Single column to display the skin name
    _treeView->AppendIconTextColumn(_("Skin"), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _treeView->AddSearchColumn(_columns.leafName);
    _treeView->SetExpandTopLevelItemsAfterPopulation(false);

	// Connect up selection changed callback
	_treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SkinChooser::_onSelChanged, this);
    _treeView->Bind(wxutil::EV_TREEVIEW_POPULATION_FINISHED, &SkinChooser::_onTreeViewPopulationFinished, this);

    _treeViewToolbar = new wxutil::ResourceTreeViewToolbar(leftPanel, _treeView);

    // Preview
    _preview.reset(new wxutil::ModelPreview(splitter));
	_preview->getWidget()->SetMinClientSize(wxSize(GetSize().GetWidth() / 3, -1));

    _materialsList = new MaterialsList(leftPanel, _preview->getRenderSystem());
    _materialsList->SetMinClientSize(wxSize(-1, 140));

    // Refresh preview when material visibility changed
    _materialsList->signal_visibilityChanged().connect(
        sigc::mem_fun(*_preview, &wxutil::ModelPreview::queueDraw)
    );

    _fileInfo = new wxutil::DeclFileInfo(leftPanel, decl::Type::Skin);

    leftPanel->GetSizer()->Add(_treeViewToolbar, 0, wxEXPAND | wxALIGN_LEFT | wxBOTTOM | wxLEFT | wxRIGHT, 6);
    leftPanel->GetSizer()->Add(_treeView, 1, wxEXPAND);
    leftPanel->GetSizer()->Add(_fileInfo, 0, wxEXPAND | wxTOP, 6);
    leftPanel->GetSizer()->Add(_materialsList, 0, wxEXPAND | wxTOP, 6);

	// Pack treeview and preview
    splitter->SplitVertically(leftPanel, _preview->getWidget());

    FitToScreen(0.8f, 0.8f);

    // Set the default size of the window
    splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.3f));

	// Overall vbox for treeview/preview and buttons
	vbox->Add(splitter, 1, wxEXPAND);
	vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxTOP, 12);

    Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &SkinChooser::_onItemActivated, this );
}

int SkinChooser::ShowModal()
{
	populateSkins();

    _treeViewToolbar->ClearFilter();

	// Display the model in the window title
	SetTitle(std::string(_(WINDOW_TITLE)) + ": " + _model);

    // Models are lazy-loaded, subscribe to the preview's event
    _modelLoadedConn.disconnect();
    _modelLoadedConn = _preview->signal_ModelLoaded().connect(
        sigc::mem_fun(this, &SkinChooser::_onPreviewModelLoaded));

	int returnCode = DialogBase::ShowModal();

	if (returnCode == wxID_OK)
	{
		// Get the selected skin
		_lastSkin = getSelectedSkin();
	}
	else
	{
		// Revert to previous skin on everything other than OK
		_lastSkin = _prevSkin;
	}

    _modelLoadedConn.disconnect();
	_preview->setModel(""); // release model

	return returnCode;
}

void SkinChooser::_onItemActivated(wxDataViewEvent& ev)
{
    // Don't respond to double clicks other than the skin tree view
    if (ev.GetEventObject() != _treeView) return;

    _lastSkin = getSelectedSkin();
    EndModal(wxID_OK);
}

// Populate the list of skins
void SkinChooser::populateSkins()
{
    _treeView->Populate(std::make_shared<ThreadedSkinLoader>(_columns, _model, _allSkinsItem, _matchingSkinsItem));
}

std::string SkinChooser::getSelectedSkin()
{
    return _treeView->GetSelectedDeclName();
}

void SkinChooser::setSelectedSkin(const std::string& skin)
{
    // Search in the matching skins tree first
    auto item = _treeView->GetTreeModel()->FindString(skin, _columns.declName, _matchingSkinsItem);

    if (!item.IsOk())
    {
        // Fall back to the All Skins tree
        item = _treeView->GetTreeModel()->FindString(skin, _columns.declName, _allSkinsItem);
    }

    if (item.IsOk())
    {
        _treeView->Select(item);
    }
    else
    {
        _treeView->UnselectAll();
    }

    handleSelectionChange();
}

// Static method to display singleton instance and choose a skin
std::string SkinChooser::chooseSkin(const std::string& model,
									const std::string& prev)
{
    auto dialog = new SkinChooser();

    dialog->_model = model;
    dialog->_prevSkin = prev;

    dialog->ShowModal();

    auto selectedSkin = dialog->_lastSkin;

    dialog->Destroy();

    return selectedSkin;
}

void SkinChooser::handleSelectionChange()
{
    auto selectedSkin = getSelectedSkin();

    // Set the model preview to show the model with the selected skin
    _preview->setModel(_model);
    _preview->setSkin(selectedSkin);

    if (!selectedSkin.empty())
    {
        _fileInfo->setName(selectedSkin);
        auto skin = GlobalModelSkinCache().findSkin(selectedSkin);
        _fileInfo->setPath(skin->getDeclFilePath());
    }
    else
    {
        _fileInfo->setName("-");
        _fileInfo->setPath("-");
    }

    updateMaterialsList();
}

void SkinChooser::updateMaterialsList()
{
    // Update the material list
    auto modelNode = Node_getModel(_preview->getModelNode());

    if (!modelNode)
    {
        _materialsList->clear();
        return;
    }

    _materialsList->updateFromModel(modelNode->getIModel());
}

void SkinChooser::_onPreviewModelLoaded(const model::ModelNodePtr& model)
{
    updateMaterialsList();
}

void SkinChooser::_onSelChanged(wxDataViewEvent& ev)
{
    handleSelectionChange();
}

void SkinChooser::_onTreeViewPopulationFinished(wxutil::ResourceTreeView::PopulationFinishedEvent& ev)
{
    // Make sure the "matching skins" item is expanded
    _treeView->Expand(_matchingSkinsItem);
    _treeView->Collapse(_allSkinsItem);

    // Select the active skin
    setSelectedSkin(_prevSkin);
}

} // namespace
