#include "ModelTreeView.h"

#include "ModelPopulator.h"

namespace ui
{

ModelTreeView::ModelTreeView(wxWindow* parent) :
    ResourceTreeView(parent, _columns, wxBORDER_STATIC | wxDV_NO_HEADER),
    _showSkins(true)
{
    // Single visible column, containing the directory/shader name and the icon
    AppendIconTextColumn(
        _("Model Path"), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE,
        wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE
    );

    // Use the TreeModel's full string search function
    AddSearchColumn(_columns.iconAndName);
    EnableFavouriteManagement(decl::Type::Model);
}

void ModelTreeView::Populate()
{
    ResourceTreeView::Populate(std::make_shared<ModelPopulator>(_columns));
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
    return GetColumnValue(_columns.modelPath);
}

std::string ModelTreeView::GetSelectedSkin()
{
    return GetColumnValue(_columns.skin);
}

bool ModelTreeView::IsTreeModelRowVisible(wxutil::TreeModel::Row& row)
{
    if (!_showSkins && row[_columns.isSkin].getBool())
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
