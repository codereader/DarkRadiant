#include "SkinChooser.h"

#include "i18n.h"
#include "modelskin.h"

#include "wxutil/Bitmap.h"
#include <wx/sizer.h>
#include <wx/splitter.h>

#include "ifavourites.h"
#include "debugging/ScopedDebugTimer.h"
#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "wxutil/dataview/VFSTreePopulator.h"

namespace ui
{

/* CONSTANTS */

namespace
{
	constexpr const char* FOLDER_ICON = "folder16.png";
	constexpr const char* SKIN_ICON = "skin16.png";

	constexpr const char* const WINDOW_TITLE = N_("Choose Skin");

    /**
     * Visitor class to retrieve skin names and add them to folders.
     */
    class ThreadedSkinLoader final :
        public wxutil::ThreadedResourceTreePopulator
    {
    private:
        const wxutil::DeclarationTreeView::Columns& _columns;

        std::set<std::string> _favourites;

        wxDataViewItem& _allSkinsItem;
        wxDataViewItem& _matchingSkinsItem;

        std::string _model;

        wxIcon _folderIcon;
        wxIcon _skinIcon;

    public:
        ThreadedSkinLoader(const wxutil::DeclarationTreeView::Columns& columns, const std::string& model, 
            wxDataViewItem& allSkinsItem, wxDataViewItem& matchingSkinsItem) :
            ThreadedResourceTreePopulator(columns),
            _columns(columns),
            _allSkinsItem(allSkinsItem),
            _matchingSkinsItem(matchingSkinsItem),
            _model(model)
        {
            // Get the list of favourites
            _favourites = GlobalFavouritesManager().getFavourites(decl::getTypeName(decl::Type::Skin));

            _skinIcon.CopyFromBitmap(wxutil::GetLocalBitmap(SKIN_ICON));
            _folderIcon.CopyFromBitmap(wxutil::GetLocalBitmap(FOLDER_ICON));
        }

        ~ThreadedSkinLoader()
        {
            EnsureStopped();
        }

    protected:
        void PopulateModel(const wxutil::TreeModel::Ptr& model) override
        {
            ScopedDebugTimer timer("ThreadedSkinLoader::run()");

            // Add the "Matching skins" toplevel node
            auto matchingSkins = model->AddItem();

            matchingSkins[_columns.iconAndName] = wxVariant(wxDataViewIconText(_("Matching skins"), _folderIcon));
            matchingSkins[_columns.fullName] = "";
            matchingSkins[_columns.isFolder] = true;
            _matchingSkinsItem = matchingSkins.getItem();

            // Get the skins for the associated model, and add them as matching skins
            const auto& matchList = GlobalModelSkinCache().getSkinsForModel(_model);

            for (const auto& skin : matchList)
            {
                auto row = model->AddItem(matchingSkins.getItem());
                StoreSkinValues(row, skin, skin, false);
            }

            // Add "All skins" toplevel node
            auto allSkins = model->AddItem();

            allSkins[_columns.iconAndName] = wxVariant(wxDataViewIconText(_("All skins"), _folderIcon));
            allSkins[_columns.fullName] = "";
            allSkins[_columns.isFolder] = true;
            _allSkinsItem = allSkins.getItem();

            // Get the list of skins for the model
            const auto& skins = GlobalModelSkinCache().getAllSkins();

            // Create a TreePopulator for the tree store and pass in each of the skin names.
            wxutil::VFSTreePopulator pop(model, allSkins.getItem());

            for (const auto& skin : skins)
            {
                pop.addPath(skin, [&](wxutil::TreeModel::Row& row,
                    const std::string& path, const std::string& leafName, bool isFolder)
                    {
                        StoreSkinValues(row, path, leafName, isFolder);
                    });
            }
        }

        void SortModel(const wxutil::TreeModel::Ptr& model) override
        {
            model->SortModelFoldersFirst(_columns.leafName, _columns.isFolder);
        }

    private:
        void StoreSkinValues(wxutil::TreeModel::Row& row, const std::string& fullSkinName, const std::string& leafName, bool isFolder)
        {
            bool isFavourite = _favourites.count(fullSkinName) > 0;

            row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, !isFolder ? _skinIcon : _folderIcon));
            row[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite);
            row[_columns.fullName] = fullSkinName;
            row[_columns.leafName] = leafName;
            row[_columns.declName] = fullSkinName;
            row[_columns.isFolder] = isFolder;
            row[_columns.isFavourite] = isFavourite;
        }
    };
}

SkinChooser::SkinChooser() :
	DialogBase(_(WINDOW_TITLE)),
	_treeView(nullptr),
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

	// Display the model in the window title
	SetTitle(std::string(_(WINDOW_TITLE)) + ": " + _model);

    // Models are lazy-loaded, subscribe to the preview's event
    _modelLoadedConn.disconnect();
    _modelLoadedConn = _preview->signal_ModelLoaded().connect(
        sigc::mem_fun(this, &SkinChooser::_onPreviewModelLoaded));

    setSelectedSkin(_prevSkin);

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
	// Get the selected skin
	wxDataViewItem item = _treeView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_treeView->GetTreeModel());
		return row[_columns.fullName];
	}
	else
	{
		return "";
	}
}

void SkinChooser::setSelectedSkin(const std::string& skin)
{
    // Search in the matching skins tree first
    auto item = _treeView->GetTreeModel()->FindString(skin, _columns.fullName, _matchingSkinsItem);

    if (!item.IsOk())
    {
        // Fall back to the All Skins tree
        item = _treeView->GetTreeModel()->FindString(skin, _columns.fullName, _allSkinsItem);
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
