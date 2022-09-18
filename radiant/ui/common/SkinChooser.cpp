#include "SkinChooser.h"

#include "i18n.h"
#include "modelskin.h"

#include <wx/sizer.h>
#include <wx/splitter.h>

#include "ifavourites.h"
#include "debugging/ScopedDebugTimer.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/decl/DeclarationSelector.h"
#include "wxutil/preview/SkinPreview.h"
#include "ui/modelselector/MaterialsList.h"

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
            // Sort the model such that the Matching Skins folder is sorted on top.
            // To achieve that we pass a custom folder sort lambda
            model->SortModelFoldersFirst(_columns.leafName, _columns.isFolder, [&] (const wxDataViewItem& a, const wxDataViewItem& b)
            {
                if (a == _matchingSkinsItem && b == _allSkinsItem)
                {
                    return -1; // matching skins on top
                }

                if (a == _allSkinsItem && b == _matchingSkinsItem)
                {
                    return +1;
                }

                // Fall back to regular string comparison for all other folders
                wxVariant aName, bName;
                model->GetValue(aName, a, _columns.leafName.getColumnIndex());
                model->GetValue(bName, b, _columns.leafName.getColumnIndex());

                return aName.GetString().CmpNoCase(bName.GetString());
            });
        }
    };
}

class SkinSelector :
    public wxutil::DeclarationSelector
{
private:
    std::string _model;

    wxDataViewItem _allSkinsItem;
    wxDataViewItem _matchingSkinsItem;

    std::unique_ptr<wxutil::SkinPreview> _preview;
    MaterialsList* _materialsList;

public:
    SkinSelector(wxWindow* parent, const std::string& model) :
        DeclarationSelector(parent, decl::Type::Skin),
        _model(model),
        _preview(new wxutil::SkinPreview(this, _model)),
        _materialsList(new MaterialsList(this, _preview->getRenderSystem()))
    {
        // We want to control ourselves which items are expanded after population
        GetTreeView()->SetExpandTopLevelItemsAfterPopulation(false);
        GetTreeView()->Bind(wxutil::EV_TREEVIEW_POPULATION_FINISHED, &SkinSelector::onTreeViewPopulationFinished, this);

        //AddPreviewToBottom(_materialsList); // TODO
        AddPreviewToRightPane(_preview.get());

        // Models are lazy-loaded, subscribe to the preview's event
        _preview->signal_ModelLoaded().connect(sigc::mem_fun(this, &SkinSelector::onPreviewModelLoaded));

        _materialsList->SetMinClientSize(wxSize(-1, 140));

        // Refresh preview when material visibility changed
        _materialsList->signal_visibilityChanged().connect(
            sigc::mem_fun(*_preview, &wxutil::ModelPreview::queueDraw)
        );
    }

    void Populate()
    {
        PopulateTreeView(std::make_shared<ThreadedSkinLoader>(GetColumns(), _model, _allSkinsItem, _matchingSkinsItem));
    }

    void SetSelectedDeclName(const std::string& skin) override
    {
        // Search in the matching skins tree first
        auto item = GetTreeView()->GetTreeModel()->FindString(skin, GetColumns().declName, _matchingSkinsItem);

        if (!item.IsOk())
        {
            // Fall back to the All Skins tree
            item = GetTreeView()->GetTreeModel()->FindString(skin, GetColumns().declName, _allSkinsItem);
        }

        if (item.IsOk())
        {
            GetTreeView()->Select(item);
            GetTreeView()->EnsureVisible(item);
        }
        else
        {
            GetTreeView()->UnselectAll();
        }
    }

private:
    void onPreviewModelLoaded(const model::ModelNodePtr& model)
    {
        updateMaterialsList();
    }

    void updateMaterialsList()
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

    void onTreeViewPopulationFinished(wxutil::ResourceTreeView::PopulationFinishedEvent& ev)
    {
        // Make sure the "matching skins" item is expanded
        GetTreeView()->Expand(_matchingSkinsItem);
        GetTreeView()->Collapse(_allSkinsItem);
    }
};

SkinChooser::SkinChooser(const std::string& model) :
	DialogBase(_(WINDOW_TITLE), "SkinChooser"),
    _model(model),
    _selector(nullptr)
{
    populateWindow();
}

void SkinChooser::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

    _selector = new SkinSelector(this, _model);

#if 0
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
    // Set the default size of the window
    splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.3f));

	// Overall vbox for treeview/preview and buttons
	vbox->Add(splitter, 1, wxEXPAND);
#endif
    vbox->Add(_selector, 1, wxEXPAND);
	vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxTOP, 12);

    Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &SkinChooser::_onItemActivated, this );

    RegisterPersistableObject(_selector);
}

int SkinChooser::ShowModal()
{
	populateSkins();

	// Display the model in the window title
	SetTitle(std::string(_(WINDOW_TITLE)) + ": " + _model);

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

	return returnCode;
}

void SkinChooser::_onItemActivated(wxDataViewEvent& ev)
{
    _lastSkin = getSelectedSkin();
    EndModal(wxID_OK);
}

// Populate the list of skins
void SkinChooser::populateSkins()
{
    _selector->Populate();
}

std::string SkinChooser::getSelectedSkin()
{
    return _selector->GetSelectedDeclName();
}

void SkinChooser::setSelectedSkin(const std::string& skin)
{
    _selector->SetSelectedDeclName(skin);
}

// Static method to display singleton instance and choose a skin
std::string SkinChooser::chooseSkin(const std::string& model,
									const std::string& prev)
{
    auto dialog = new SkinChooser(model);

    dialog->_model = model;
    dialog->_prevSkin = prev;

    dialog->ShowModal();

    auto selectedSkin = dialog->_lastSkin;

    dialog->Destroy();

    return selectedSkin;
}

void SkinChooser::_onTreeViewPopulationFinished(wxutil::ResourceTreeView::PopulationFinishedEvent& ev)
{
    // Select the active skin
    setSelectedSkin(_prevSkin);
}

} // namespace
