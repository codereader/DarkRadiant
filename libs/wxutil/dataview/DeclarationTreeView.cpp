#include "DeclarationTreeView.h"

#include "i18n.h"
#include "ideclmanager.h"
#include "wxutil/sourceview/DeclarationSourceView.h"
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
    SetFavouriteKeyColumn(_columns.declName);
    EnableFavouriteManagement(decl::getTypeName(_declType));
}

std::string DeclarationTreeView::GetSelectedDeclName()
{
    return !IsDirectorySelected() ? GetSelectedElement(_columns.declName) : std::string();
}

void DeclarationTreeView::SetSelectedDeclName(const std::string& declName)
{
    SetSelectedElement(declName, _columns.declName);
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

DeclarationSourceView* DeclarationTreeView::CreateDeclarationView(const decl::IDeclaration::Ptr& decl)
{
    auto view = new DeclarationSourceView(this);
    view->setDeclaration(decl);

    return view;
}

std::string DeclarationTreeView::GetResourcePath(const TreeModel::Row& row)
{
    return row[_columns.declName];
}

void DeclarationTreeView::_onShowDefinition()
{
    auto declName = GetSelectedDeclName();

    if (auto decl = GlobalDeclarationManager().findDeclaration(_declType, declName); decl)
    {
        auto* view = CreateDeclarationView(decl);
        view->ShowModal();
        view->Destroy();
    }
}

bool DeclarationTreeView::_showDefinitionEnabled()
{
    return !GetSelectedDeclName().empty();
}

}
