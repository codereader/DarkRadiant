#include "ModelTreeView.h"

#include "ModelPopulator.h"

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
    EnableFavouriteManagement(decl::Type::Model);
}

const ModelTreeView::TreeColumns& ModelTreeView::Columns() const
{
    static TreeColumns _treeColumns;
    return _treeColumns;
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

}
