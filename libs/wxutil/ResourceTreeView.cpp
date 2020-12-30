#include "ResourceTreeView.h"

#include "i18n.h"
#include "ifavourites.h"
#include "menu/IconTextMenuItem.h"
#include "TreeViewItemStyle.h"

namespace wxutil
{

namespace
{
    const char* const ADD_TO_FAVOURITES = N_("Add to Favourites");
    const char* const REMOVE_FROM_FAVOURITES = N_("Remove from Favourites");
}

ResourceTreeView::ResourceTreeView(wxWindow* parent, const ResourceTreeView::Columns& columns, long style) :
    ResourceTreeView(parent, nullptr, columns, style)
{}

ResourceTreeView::ResourceTreeView(wxWindow* parent, wxDataViewModel* model, 
                                   const ResourceTreeView::Columns& columns, long style) :
    TreeView(parent, model, style),
    _columns(columns)
{
    Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &ResourceTreeView::_onContextMenu, this);
}

void ResourceTreeView::populateContextMenu(wxutil::PopupMenu& popupMenu)
{
    if (popupMenu.GetMenuItemCount() > 0)
    {
        popupMenu.addSeparator();
    }

    popupMenu.addItem(
        new StockIconTextMenuItem(_(ADD_TO_FAVOURITES), wxART_ADD_BOOKMARK),
        std::bind(&ResourceTreeView::_onSetFavourite, this, true),
        std::bind(&ResourceTreeView::_testAddToFavourites, this)
    );

    popupMenu.addItem(
        new StockIconTextMenuItem(_(REMOVE_FROM_FAVOURITES), wxART_DEL_BOOKMARK),
        std::bind(&ResourceTreeView::_onSetFavourite, this, false),
        std::bind(&ResourceTreeView::_testRemoveFromFavourites, this)
    );
}

std::string ResourceTreeView::getSelection()
{
    // Get the selected value
    wxDataViewItem item = GetSelection();

    if (!item.IsOk())
    {
        return std::string(); // nothing selected
    }

    // Cast to TreeModel::Row and get the full name
    TreeModel::Row row(item, *GetModel());

    return row[_columns.fullName];
}

void ResourceTreeView::_onContextMenu(wxDataViewEvent& ev)
{
    if (!_popupMenu)
    {
        _popupMenu.reset(new wxutil::PopupMenu());

        populateContextMenu(*_popupMenu);
    }

    _popupMenu->show(this);
}

bool ResourceTreeView::_testAddToFavourites()
{
    // Adding favourites is allowed for any folder and non-favourite items 
    return isDirectorySelected() || (GetSelection().IsOk() && !isFavouriteSelected());
}

bool ResourceTreeView::_testRemoveFromFavourites()
{
    // We can run remove from favourites on any folder or on favourites themselves
    return isDirectorySelected() || isFavouriteSelected();
}

void ResourceTreeView::setFavouriteRecursively(TreeModel::Row& row, bool isFavourite)
{
    if (row[_columns.isFolder].getBool())
    {
        // Enter recursion for this folder
        wxDataViewItemArray children;
        GetModel()->GetChildren(row.getItem(), children);

        for (const wxDataViewItem& child : children)
        {
            TreeModel::Row childRow(child, *GetModel());
            setFavouriteRecursively(childRow, isFavourite);
        }

        return;
    }

    // Not a folder, set the desired status on this item
    row[_columns.isFavourite] = isFavourite;
    row[_columns.iconAndName] = TreeViewItemStyle::Declaration(isFavourite);

    // Keep track of this choice
    if (isFavourite)
    {
        GlobalFavouritesManager().addFavourite(decl::Type::Material, row[_columns.fullName]);
    }
    else
    {
        GlobalFavouritesManager().removeFavourite(decl::Type::Material, row[_columns.fullName]);
    }

    row.SendItemChanged();
}

void ResourceTreeView::_onSetFavourite(bool isFavourite)
{
    wxDataViewItem item = GetSelection();

    if (!item.IsOk()) return;

    // Grab this item and enter recursion, propagating the favourite status
    TreeModel::Row row(item, *GetModel());

    setFavouriteRecursively(row, isFavourite);

    // Store to registry on each change
    // TODO: ? _favourites->saveToRegistry();
}

bool ResourceTreeView::isDirectorySelected()
{
    wxDataViewItem item = GetSelection();

    if (!item.IsOk()) return false;

    TreeModel::Row row(item, *GetModel());

    return row[_columns.isFolder].getBool();
}

bool ResourceTreeView::isFavouriteSelected()
{
    wxDataViewItem item = GetSelection();

    if (!item.IsOk()) return false;

    TreeModel::Row row(item, *GetModel());

    return row[_columns.isFavourite].getBool();
}

}
