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
    _declType(declType)
{
    EnableFavouriteManagement(decl::getTypeName(_declType));
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
    auto fullName = GetSelectedFullname();
    auto decl = GlobalDeclarationManager().findDeclaration(_declType, fullName);

    if (decl)
    {
        auto* view = new DeclarationSourceView(decl, this);
        view->ShowModal();
        view->Destroy();
    }
}

bool DeclarationTreeView::_showDefinitionEnabled()
{
    return !GetSelectedFullname().empty();
}

}
