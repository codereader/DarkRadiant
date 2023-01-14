#include "SkinChooser.h"

#include "i18n.h"
#include "modelskin.h"

#include "fmt/format.h"

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
    std::string _skinToSelectAfterPopulation;

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

        AddWidgetToBottom(_materialsList);
        AddPreviewToRightPane(_preview.get());

        // Models are lazy-loaded, subscribe to the preview's event
        _preview->signal_ModelLoaded().connect(sigc::mem_fun(this, &SkinSelector::onPreviewModelLoaded));

        _materialsList->SetMinClientSize(wxSize(-1, 140));

        // Refresh preview when material visibility changed
        _materialsList->signal_visibilityChanged().connect(
            sigc::mem_fun(*_preview, &wxutil::ModelPreview::queueDraw)
        );

        Populate();
    }

    void Populate() override
    {
        PopulateTreeView(std::make_shared<ThreadedSkinLoader>(GetColumns(), _model, _allSkinsItem, _matchingSkinsItem));
    }

    void SetSelectedDeclName(const std::string& skin) override
    {
        _skinToSelectAfterPopulation = skin;

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

        _preview->SetPreviewDeclName(GetSelectedDeclName());
        updateMaterialsList();
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
        FocusTreeView();

        // Make sure the "matching skins" item is expanded
        GetTreeView()->Expand(_matchingSkinsItem);
        GetTreeView()->Collapse(_allSkinsItem);

        if (!_skinToSelectAfterPopulation.empty())
        {
            SetSelectedDeclName(_skinToSelectAfterPopulation);
            _skinToSelectAfterPopulation.clear();
        }

        ev.Skip();
    }
};

SkinChooser::SkinChooser(const std::string& model) :
    DeclarationSelectorDialog(decl::Type::Skin, _(WINDOW_TITLE), "SkinChooser"),
    _model(model)
{
    SetSelector(new SkinSelector(this, _model));
}

int SkinChooser::ShowModal()
{
	// Display the model in the window title
	SetTitle(fmt::format("{0}: {1}", _(WINDOW_TITLE), _model));

	return DeclarationSelectorDialog::ShowModal();
}

std::string SkinChooser::ChooseSkin(const std::string& model, const std::string& prev)
{
    auto dialog = new SkinChooser(model);

    // We return the previous skin, unless the dialog returns OK
    auto selectedSkin = prev;

    dialog->SetSelectedDeclName(selectedSkin);

    if (dialog->ShowModal() == wxID_OK)
    {
        selectedSkin = dialog->GetSelectedDeclName();
    }

    dialog->Destroy();

    return selectedSkin;
}

} // namespace
