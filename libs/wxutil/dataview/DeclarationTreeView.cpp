#include "DeclarationTreeView.h"

#include "i18n.h"
#include "ideclmanager.h"
#include "wxutil/DeclarationSourceView.h"
#include "wxutil/menu/IconTextMenuItem.h"

namespace wxutil
{

DeclarationTreeView::DeclarationTreeView(wxWindow* parent, decl::Type declType, const Columns& columns, long style) :
    DeclarationTreeView(parent, declType, TreeModel::Ptr(), columns, style)
{}

DeclarationTreeView::DeclarationTreeView(wxWindow* parent, decl::Type declType, const TreeModel::Ptr& model, const Columns& columns, long style) :
    ResourceTreeView(parent, model, columns, style),
    _columns(columns),
    _declType(declType)
{
    EnableFavouriteManagement(decl::getTypeName(_declType));
}

std::string DeclarationTreeView::GetSelectedDeclName()
{
    auto item = GetSelection();

    if (!item.IsOk() || IsDirectorySelected())
    {
        return std::string();
    }

    TreeModel::Row row(item, *GetModel());
    return row[_columns.declName];
}

void DeclarationTreeView::PopulateContextMenu(wxutil::PopupMenu& popupMenu)
{
    ResourceTreeView::PopulateContextMenu(popupMenu);

    // Add one more item at the bottom of the popupmenu
    popupMenu.addItem(
        new wxutil::IconTextMenuItem(_("Show Definition"), "decl.png"),
        std::bind(&DeclarationTreeView::_onShowDefinition, this),
        std::bind(&DeclarationTreeView::_showDefinitionEnabled, this),
        [this] { return _declType != decl::Type::None; }
    );
}

void DeclarationTreeView::_onShowDefinition()
{
    auto declName = GetSelectedDeclName();
    auto decl = GlobalDeclarationManager().findDeclaration(_declType, declName);

    if (decl)
    {
        auto* view = new DeclarationSourceView(decl, this);
        view->ShowModal();
        view->Destroy();
    }
}

bool DeclarationTreeView::_showDefinitionEnabled()
{
    return !IsDirectorySelected() && !GetSelectedDeclName().empty();
}

}
