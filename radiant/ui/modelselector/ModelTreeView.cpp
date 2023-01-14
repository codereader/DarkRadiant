#include "ModelTreeView.h"

#include "ideclmanager.h"
#include "ModelPopulator.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/sourceview/DeclarationSourceView.h"

namespace ui
{

ModelTreeView::ModelTreeView(wxWindow* parent) :
    ResourceTreeView(parent, Columns(), wxBORDER_STATIC | wxDV_NO_HEADER),
    _showSkins(true)
{
    // Single visible column, containing the directory/shader name and the icon
    AppendIconTextColumn(
        _("Model Path"), Columns().iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE,
        wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE
    );

    // Use the TreeModel's full string search function
    AddSearchColumn(Columns().iconAndName);
    EnableFavouriteManagement("model");

    AddCustomMenuItem(std::make_shared<wxutil::MenuItem>(
        new wxutil::IconTextMenuItem(_("Show Definition"), "decl.png"),
        std::bind(&ModelTreeView::showDefinition, this),
        std::bind(&ModelTreeView::testShowDefinition, this)));
}

const ModelTreeView::TreeColumns& ModelTreeView::Columns() const
{
    static TreeColumns _treeColumns;
    return _treeColumns;
}

std::string ModelTreeView::GetResourcePath(const wxutil::TreeModel::Row& row)
{
    return row[Columns().isSkin].getBool() ? row[Columns().skin] : row[Columns().modelPath];
}

void ModelTreeView::Populate()
{
    ResourceTreeView::Populate(std::make_shared<ModelPopulator>(Columns()));
}

void ModelTreeView::SetShowSkins(bool showSkins)
{
    if (_showSkins == showSkins)
    {
        return;
    }

    // Try to keep the selection intact when switching modes
    auto previousSelection = GetSelectedFullname();

    _showSkins = showSkins;

    SetupTreeModelFilter(); // refresh the view

    // The modelDefs folder should start in collapsed state
    CollapseModelDefsFolder();

    if (!previousSelection.empty())
    {
        SetSelectedFullname(previousSelection);
    }
}

std::string ModelTreeView::GetSelectedModelPath()
{
    return GetColumnValue(Columns().modelPath);
}

std::string ModelTreeView::GetSelectedSkin()
{
    return GetColumnValue(Columns().skin);
}

void ModelTreeView::SetSelectedSkin(const std::string& skin)
{
    if (!_showSkins) return;

    auto selectedModel = GetSelectedModelPath();

    if (selectedModel.empty()) return;

    // Search the subtree of the current selection, starting from the parent
    // (we might already have selected a skin, so start at the parent to be safe).
    auto matchingItem = GetTreeModel()->FindItem([&](const wxutil::TreeModel::Row& row)
    {
        std::string foundModel = row[Columns().modelPath];
        std::string foundSkin = row[Columns().skin];

        return foundModel == selectedModel && foundSkin == skin;
    }, GetTreeModel()->GetParent(GetSelection()));

    if (matchingItem.IsOk())
    {
        Select(matchingItem);
        EnsureVisible(matchingItem);
    }
}

void ModelTreeView::CollapseModelDefsFolder()
{
    // Find the modelDefs item in the top-level children and collapse it
    wxDataViewItemArray children;
    GetTreeModel()->GetChildren(GetTreeModel()->GetRoot(), children);

    for (auto& item : children)
    {
        wxutil::TreeModel::Row row(item, *GetModel());

        if (row[Columns().isModelDefFolder].getBool())
        {
            Collapse(item);
            break;
        }
    }
}

bool ModelTreeView::IsTreeModelRowVisible(wxutil::TreeModel::Row& row)
{
    if (!_showSkins && row[Columns().isSkin].getBool())
    {
        return false; // it's a skin, and we shouldn't show it
    }

    // Pass to the base class
    return ResourceTreeView::IsTreeModelRowVisible(row);
}

std::string ModelTreeView::GetColumnValue(const wxutil::TreeModel::Column& column)
{
    auto item = GetSelection();

    if (!item.IsOk()) return "";

    wxutil::TreeModel::Row row(item, *GetModel());

    return row[column];
}

void ModelTreeView::showDefinition()
{
    auto item = GetSelection();
    wxutil::TreeModel::Row row(item, *GetModel());

    decl::IDeclaration::Ptr decl;

    if (row[Columns().isSkin].getBool())
    {
        decl = GlobalDeclarationManager().findDeclaration(decl::Type::Skin, GetSelectedSkin());
    }
    else
    {
        // Must be a modelDef
        auto selectedModelPath = row[Columns().modelPath].getString().ToStdString();
        decl = GlobalDeclarationManager().findDeclaration(decl::Type::ModelDef, selectedModelPath);
    }

    if (decl)
    {
        auto view = new wxutil::DeclarationSourceView(this);
        view->setDeclaration(decl);

        view->ShowModal();
        view->Destroy();
    }
}

bool ModelTreeView::testShowDefinition()
{
    if (IsDirectorySelected()) return false;

    auto item = GetSelection();
    if (!item.IsOk()) return false;

    wxutil::TreeModel::Row row(item, *GetModel());

    auto selectedModelPath = row[Columns().modelPath].getString().ToStdString();

    if (GlobalDeclarationManager().findDeclaration(decl::Type::ModelDef, selectedModelPath))
    {
        return true;
    }

    // Could also be a skin
    return _showSkins && row[Columns().isSkin].getBool();
}

}
