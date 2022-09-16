#include "DeclarationSelector.h"

namespace ui
{

DeclarationSelector::DeclarationSelector(wxWindow* parent, decl::Type declType, 
        const wxutil::DeclarationTreeView::Columns& columns) :
    wxPanel(parent),
    _declType(declType),
    _columns(columns),
    _treeView(nullptr)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));
    createTreeView();
}

void DeclarationSelector::createTreeView()
{
    _treeView = new wxutil::DeclarationTreeView(this, decl::Type::Material,
        _columns, wxDV_NO_HEADER | wxDV_SINGLE);

    // Single visible column, containing the directory/decl name and the icon
    _treeView->AppendIconTextColumn(_("Value"), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Use the TreeModel's full string search function
    _treeView->AddSearchColumn(_columns.leafName);

    // Get selection and connect the changed callback
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DeclarationSelector::onTreeViewSelectionChanged, this);

    GetSizer()->Add(_treeView, 1, wxEXPAND);
}

wxutil::DeclarationTreeView* DeclarationSelector::GetTreeView()
{
    return _treeView;
}

void DeclarationSelector::PopulateTreeView(const wxutil::IResourceTreePopulator::Ptr& populator)
{
    _treeView->Populate(populator);
}

void DeclarationSelector::onTreeViewSelectionChanged(wxDataViewEvent& ev)
{
    ev.Skip();
}

}
