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
    const char* const ICON_LOADING = "icon_classname.png";
}

ResourceTreeView::ResourceTreeView(wxWindow* parent, const ResourceTreeView::Columns& columns, long style) :
    ResourceTreeView(parent, TreeModel::Ptr(), columns, style)
{}

ResourceTreeView::ResourceTreeView(wxWindow* parent, const TreeModel::Ptr& model,
                                   const ResourceTreeView::Columns& columns, long style) :
    TreeView(parent, nullptr, style), // associate the model later
    _columns(columns),
    _mode(TreeMode::ShowAll)
{
    // Note that we need to avoid accessing the _columns reference in the constructor
    // since it is likely owned by subclasses and might not be ready yet

    _treeStore = model;

    if (!_treeStore)
    {
        // Create an empty default model
        _treeStore.reset(new TreeModel(_columns));
    }

    AssociateModel(_treeStore.get());

    Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &ResourceTreeView::_onContextMenu, this);
}

const TreeModel::Ptr& ResourceTreeView::getTreeModel()
{
    return _treeStore;
}

void ResourceTreeView::setTreeModel(const TreeModel::Ptr& model)
{
    _treeStore = model;
    _emptyFavouritesLabel = wxDataViewItem();

    if (!_treeStore)
    {
        _treeModelFilter = TreeModelFilter::Ptr();
        AssociateModel(nullptr);
        return;
    }

    setupTreeModelFilter();
}

void ResourceTreeView::setupTreeModelFilter()
{
    // Set up the filter
    _treeModelFilter.reset(new TreeModelFilter(_treeStore));

    _treeModelFilter->SetVisibleFunc([this](TreeModel::Row& row)
    {
        return treeModelFilterFunc(row);
    });

    AssociateModel(_treeModelFilter.get());

    // Remove the dummy label in any case
    if (_emptyFavouritesLabel.IsOk())
    {
        _treeStore->RemoveItem(_emptyFavouritesLabel);
        _emptyFavouritesLabel = wxDataViewItem();
    }

    if (_mode == TreeMode::ShowFavourites)
    {
        wxDataViewItemArray visibleChildren;
        if (_treeModelFilter->GetChildren(_treeModelFilter->GetRoot(), visibleChildren) == 0)
        {
            // All items filtered out, show the dummy label
            if (!_emptyFavouritesLabel.IsOk())
            {
                wxutil::TreeModel::Row row = _treeStore->AddItem();
                _emptyFavouritesLabel = row.getItem();

                wxIcon icon;
                icon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + ICON_LOADING));
                row[_columns.iconAndName] = wxVariant(wxDataViewIconText(_("No favourites added so far"), icon));
                row[_columns.isFavourite] = true;
                row[_columns.isFolder] = false;

                row.SendItemAdded();
            }
        }

        ExpandTopLevelItems();
    }
}

ResourceTreeView::TreeMode ResourceTreeView::getTreeMode() const
{
    return _mode;
}

void ResourceTreeView::setTreeMode(ResourceTreeView::TreeMode mode)
{
    _mode = mode;

    setupTreeModelFilter();
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

void ResourceTreeView::setSelection(const std::string& fullName)
{
    // If the selection string is empty, collapse the treeview and return with
    // no selection
    if (fullName.empty())
    {
        Collapse(getTreeModel()->GetRoot());
        return;
    }

    // Find the requested element
    auto item = getTreeModel()->FindString(fullName, _columns.fullName);

    if (item.IsOk())
    {
        Select(item);
        EnsureVisible(item);

        // Send a selection change event
        wxDataViewEvent ev(wxEVT_DATAVIEW_SELECTION_CHANGED, this, item);
        ProcessWindowEvent(ev);
    }
}

void ResourceTreeView::clear()
{
    // Clear the media browser on MaterialManager unrealisation
    _treeStore->Clear();
    _emptyFavouritesLabel = wxDataViewItem();
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

bool ResourceTreeView::treeModelFilterFunc(wxutil::TreeModel::Row& row)
{
    if (_mode == TreeMode::ShowAll) return true; // everything is visible

    // Favourites mode, check if this item or any descendant is visible
    if (row[_columns.isFavourite].getBool())
    {
        return true;
    }

    wxDataViewItemArray children;
    _treeStore->GetChildren(row.getItem(), children);

    // Enter the recursion for each of the children and bail out on the first visible one
    for (const wxDataViewItem& child : children)
    {
        wxutil::TreeModel::Row childRow(child, *_treeStore);

        if (treeModelFilterFunc(childRow))
        {
            return true;
        }
    }

    return false;
}

}
