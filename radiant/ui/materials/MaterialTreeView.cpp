#include "MaterialTreeView.h"

#include "i18n.h"
#include "MaterialPopulator.h"

namespace ui
{

MaterialTreeView::MaterialTreeView(wxWindow* parent) :
    ResourceTreeView(parent, Columns(), wxDV_NO_HEADER)
{
    auto* textCol = AppendIconTextColumn(_("Shader"), Columns().iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

    SetExpanderColumn(textCol);
    textCol->SetWidth(300);

    AddSearchColumn(Columns().iconAndName);
    EnableFavouriteManagement(decl::Type::Material);

    // The wxWidgets algorithm sucks at sorting large flat lists of strings,
    // so we do that ourselves
    GetTreeModel()->SetHasDefaultCompare(false);
}

const MaterialTreeView::TreeColumns& MaterialTreeView::Columns() const
{
    static TreeColumns _treeColumns;
    return _treeColumns;
}

void MaterialTreeView::Populate()
{
    ResourceTreeView::Populate(std::make_shared<MaterialPopulator>(Columns()));
}

void MaterialTreeView::SetTreeMode(MaterialTreeView::TreeMode mode)
{
    std::string previouslySelectedItem = GetSelectedFullname();

    ResourceTreeView::SetTreeMode(mode);

    // Try to select the same item we had as before
    SetSelectedFullname(previouslySelectedItem);
}

}
