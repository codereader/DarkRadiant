#include "ResourceTreeView.h"

#include "i18n.h"
#include "iclipboard.h"
#include "ifavourites.h"
#include "../menu/IconTextMenuItem.h"
#include "TreeViewItemStyle.h"
#include "wxutil/Bitmap.h"

namespace wxutil
{

namespace
{
    const char* const ADD_TO_FAVOURITES = N_("Add to Favourites");
    const char* const REMOVE_FROM_FAVOURITES = N_("Remove from Favourites");
    const char* const ICON_LOADING = "icon_classname.png";
}

// Event implementation
ResourceTreeView::PopulationFinishedEvent::PopulationFinishedEvent(int id) :
    wxEvent(id, EV_TREEVIEW_POPULATION_FINISHED)
{
    m_propagationLevel = wxEVENT_PROPAGATE_MAX;
}

ResourceTreeView::PopulationFinishedEvent::PopulationFinishedEvent(const PopulationFinishedEvent& event) :
    wxEvent(event)
{}

wxEvent* ResourceTreeView::PopulationFinishedEvent::Clone() const
{
    return new PopulationFinishedEvent(*this);
}

wxDEFINE_EVENT(EV_TREEVIEW_POPULATION_FINISHED, ResourceTreeView::PopulationFinishedEvent);
wxDEFINE_EVENT(EV_TREEVIEW_FILTERTEXT_CLEARED, wxCommandEvent);

ResourceTreeView::ResourceTreeView(wxWindow* parent, const ResourceTreeView::Columns& columns, long style) :
    ResourceTreeView(parent, TreeModel::Ptr(), columns, style)
{}

ResourceTreeView::ResourceTreeView(wxWindow* parent, const TreeModel::Ptr& model,
                                   const ResourceTreeView::Columns& columns, long style) :
    TreeView(parent, nullptr, style), // associate the model later
    _columns(columns),
    _mode(TreeMode::ShowAll),
    _progressIcon(GetLocalBitmap(ICON_LOADING)),
    _expandTopLevelItemsAfterPopulation(false),
    _columnToSelectAfterPopulation(nullptr),
    _setFavouritesRecursively(true),
    _declPathColumn(_columns.fullName),
    _favouriteKeyColumn(_columns.fullName)
{
    _treeStore = model;

    if (!_treeStore)
    {
        // Create an empty default model
        _treeStore.reset(new TreeModel(_columns));
    }

    AssociateModel(_treeStore.get());

    Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &ResourceTreeView::_onContextMenu, this);
    Bind(EV_TREEMODEL_POPULATION_FINISHED, &ResourceTreeView::_onTreeStorePopulationFinished, this);
    Bind(EV_TREEMODEL_POPULATION_PROGRESS, &ResourceTreeView::_onTreeStorePopulationProgress, this);
}

ResourceTreeView::~ResourceTreeView()
{
    if (_populator)
    {
        // This call isn't strictly necessary if the implementing thread
        // populators are equipped with proper destructors.
        // In case some user code is missing such a CancelAndWait call in their
        // destructor, let's call it here while the object is still intact.
        _populator->EnsureStopped();
        _populator.reset();
    }
}

const TreeModel::Ptr& ResourceTreeView::GetTreeModel()
{
    return _treeStore;
}

void ResourceTreeView::SetTreeModel(const TreeModel::Ptr& model)
{
    _treeStore = model;
    _emptyFavouritesLabel = wxDataViewItem();

    if (!_treeStore)
    {
        _treeModelFilter = TreeModelFilter::Ptr();
        AssociateModel(nullptr);
        return;
    }

    SetupTreeModelFilter();
}

void ResourceTreeView::SetupTreeModelFilter()
{
    // Set up the filter
    _treeModelFilter.reset(new TreeModelFilter(_treeStore));

    _treeModelFilter->SetVisibleFunc(
        std::bind(&ResourceTreeView::IsTreeModelRowOrAnyChildVisible, this, std::placeholders::_1));

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

                row[_columns.iconAndName] = wxVariant(wxDataViewIconText(_("No favourites added so far"), _progressIcon));
                row[_columns.isFavourite] = true;
                row[_columns.isFolder] = false;

                row.SendItemAdded();
            }
        }
    }

    ExpandTopLevelItems();
}

ResourceTreeView::TreeMode ResourceTreeView::GetTreeMode() const
{
    return _mode;
}

void ResourceTreeView::SetTreeMode(ResourceTreeView::TreeMode mode)
{
    if (_mode == mode) return;

    // Try to keep the selection intact when switching modes
    auto previousSelection = GetSelectedFullname();

    _mode = mode;

    SetupTreeModelFilter();

    if (!previousSelection.empty())
    {
        SetSelectedFullname(previousSelection);
    }
}

void ResourceTreeView::PopulateContextMenu(wxutil::PopupMenu& popupMenu)
{
    // Add the pre-registered items first (with an auto-separator)
    if (popupMenu.GetMenuItemCount() > 0)
    {
        popupMenu.addSeparator();
    }

    for (const auto& customItem : _customMenuItems)
    {
        popupMenu.addItem(customItem);
    }

    _customMenuItems.clear();

    if (popupMenu.GetMenuItemCount() > 0)
    {
        popupMenu.addSeparator();
    }

    popupMenu.addItem(
        new StockIconTextMenuItem(_(ADD_TO_FAVOURITES), wxART_ADD_BOOKMARK),
        std::bind(&ResourceTreeView::_onSetFavourite, this, true),
        std::bind(&ResourceTreeView::_testAddToFavourites, this),
        [this]() { return !_favouriteTypeName.empty(); }
    );

    popupMenu.addItem(
        new StockIconTextMenuItem(_(REMOVE_FROM_FAVOURITES), wxART_DEL_BOOKMARK),
        std::bind(&ResourceTreeView::_onSetFavourite, this, false),
        std::bind(&ResourceTreeView::_testRemoveFromFavourites, this),
        [this]() { return !_favouriteTypeName.empty(); }
    );

    popupMenu.addSeparator();

    popupMenu.addItem(
        new wxutil::StockIconTextMenuItem(_("Copy Resource Path"), wxART_COPY),
        std::bind(&ResourceTreeView::_onCopyResourcePath, this),
        std::bind(&ResourceTreeView::_copyResourcePathEnabled, this),
        std::bind(&ResourceTreeView::_copyResourcePathVisible, this)
    );
}

bool ResourceTreeView::SetFilterText(const wxString& filterText)
{
    // We use the lower-case copy of the given filter text
    _filterText = filterText.Lower();

    wxDataViewItem item = GetSelection();

    // Update the top level tree items which rebuilds the view
    UpdateTreeVisibility();

    // Keep the previous selection if not filtered out and is meaningful
    if (item.IsOk() && _treeModelFilter->ItemIsVisible(item))
    {
        TreeModel::Row row(item, *GetModel());

        if (!_filterText.empty() && !TreeModel::RowContainsString(row, _filterText, _colsToSearch, true))
        {
            // The selected row is not relevant anymore
            return JumpToFirstFilterMatch();
        }

        // Try to keep whatever selection we had before
        Select(item);
        EnsureVisible(item);
        return true;
    }
     
    return JumpToFirstFilterMatch();
}

void ResourceTreeView::UpdateTreeVisibility()
{
    if (_treeModelFilter)
    {
#if defined(__WXGTK__) && !wxCHECK_VERSION(3, 0, 5)
        // In wxGTK 3.0.4 Cleared() will just wipe out the treeview
        // Re-associate the model to refresh the view
        AssociateModel(nullptr);
        AssociateModel(_treeModelFilter.get());
#else
        // Notify the attached view that it should reload
        _treeModelFilter->Cleared();
#endif
    }
}

bool ResourceTreeView::JumpToFirstFilterMatch()
{
    if (_filterText.empty() || !_treeModelFilter) return false;

    auto item = _treeModelFilter->FindNextString(_filterText, _colsToSearch);

    if (item.IsOk())
    {
        JumpToSearchMatch(item);
        return true;
    }
    
    return false;
}

void ResourceTreeView::JumpToNextFilterMatch()
{
    if (_filterText.empty()) return;

    auto selectedItem = GetSelection();
    auto item = _treeModelFilter->FindNextString(_filterText, _colsToSearch, selectedItem);

    if (item.IsOk())
    {
        JumpToSearchMatch(item);
    }
}

void ResourceTreeView::JumpToPrevFilterMatch()
{
    if (_filterText.empty()) return;

    auto selectedItem = GetSelection();
    auto item = _treeModelFilter->FindPrevString(_filterText, _colsToSearch, selectedItem);

    if (item.IsOk())
    {
        JumpToSearchMatch(item);
    }
}

void ResourceTreeView::ClearFilterText()
{
    _filterText.clear();

    UpdateTreeVisibility();

    wxQueueEvent(this, new wxCommandEvent(EV_TREEVIEW_FILTERTEXT_CLEARED));
}

std::string ResourceTreeView::GetSelectedFullname()
{
    return GetSelectedElement(_columns.fullName);
}

void ResourceTreeView::SetSelectedFullname(const std::string& fullName)
{
    SetSelectedElement(fullName, _columns.fullName);
}

std::string ResourceTreeView::GetSelectedElement(const TreeModel::Column& column)
{
    // Get the selected value
    wxDataViewItem item = GetSelection();

    if (!item.IsOk())
    {
        return std::string(); // nothing selected
    }

    // Cast to TreeModel::Row and get the full name
    TreeModel::Row row(item, *GetModel());

    return row[column];
}

void ResourceTreeView::SetSelectedElement(const std::string& value, const TreeModel::Column& column)
{
    if (_populator)
    {
        // Postpone the selection, store the name
        _elementToSelectAfterPopulation = value;
        _columnToSelectAfterPopulation = &column;
        return;
    }

    // If the selection string is empty, collapse the treeview and return with
    // no selection
    if (value.empty())
    {
        Collapse(GetTreeModel()->GetRoot());
        return;
    }

    // Clear the filter before seeking an element
    ClearFilterText();

    // Find the requested element
    auto item = GetTreeModel()->FindString(value, column);

    if (item.IsOk())
    {
        Select(item);
        EnsureVisible(item);

        // Send an artificial selection change event
        SendSelectionChangeEvent(item);
    }

    _elementToSelectAfterPopulation.clear();
    _columnToSelectAfterPopulation = nullptr;
}

void ResourceTreeView::Clear()
{
#ifdef __WXGTK__
    // Clear the m_ensureVisibleDefered owned by the wxDataViewCtrl
    EnsureVisible(wxDataViewItem(nullptr));
#endif

    // Clear any data and/or active population objects
    _populator.reset();
    _treeStore->Clear();
    _emptyFavouritesLabel = wxDataViewItem();
}

void ResourceTreeView::EnableFavouriteManagement(const std::string& typeName)
{
    _favouriteTypeName = typeName;
}

void ResourceTreeView::DisableFavouriteManagement()
{
    _favouriteTypeName.clear();
    SetTreeMode(TreeMode::ShowAll);
}

void ResourceTreeView::Populate(const IResourceTreePopulator::Ptr& populator)
{
    // Try to keep the current selection intact after population
    _elementToSelectAfterPopulation = GetSelectedFullname();
    _columnToSelectAfterPopulation = &_columns.fullName;

    // Remove any data or running populators first
    Clear();

    // Add the loading icon to the tree
    TreeModel::Row row = GetTreeModel()->AddItem();

    row[_columns.iconAndName] = wxVariant(wxDataViewIconText(_("Loading resources..."), _progressIcon));
    row[_columns.isFavourite] = true;
    row[_columns.isFolder] = false;
    _progressItem = row.getItem();

    row.SendItemAdded();

    // It will fire the TreeModel::PopulationFinishedEvent when done, point it to ourselves
    populator->SetFinishedHandler(this);

    // Start population (this might be a thread or not)
    _populator = populator;
    _populator->Populate();
}

void ResourceTreeView::SetExpandTopLevelItemsAfterPopulation(bool expand)
{
    _expandTopLevelItemsAfterPopulation = expand;
}

void ResourceTreeView::AddCustomMenuItem(const ui::IMenuItemPtr& item)
{
    _customMenuItems.push_back(item);
}

void ResourceTreeView::_onTreeStorePopulationProgress(TreeModel::PopulationProgressEvent& ev)
{
    if (!_progressItem.IsOk()) return;

    wxutil::TreeModel::Row row(_progressItem, *GetModel());
    row[_columns.iconAndName] = wxVariant(wxDataViewIconText(ev.GetMessage(), _progressIcon));
    row.SendItemChanged();
}

void ResourceTreeView::_onTreeStorePopulationFinished(TreeModel::PopulationFinishedEvent& ev)
{
    UnselectAll();
    SetTreeModel(ev.GetTreeModel());
    _populator.reset();
    _progressItem = wxDataViewItem();

    // Trigger a column size event on the first-level row
    TriggerColumnSizeEvent();

    if (_expandTopLevelItemsAfterPopulation)
    {
        ExpandTopLevelItems();
    }

    // Populator is empty now, check if we need to pre-select anything
    if (!_elementToSelectAfterPopulation.empty())
    {
        if (_columnToSelectAfterPopulation == nullptr)
        {
            _columnToSelectAfterPopulation = &_columns.fullName;
        }

        SetSelectedElement(_elementToSelectAfterPopulation, *_columnToSelectAfterPopulation);
    }

    // Emit the finished event
    wxQueueEvent(this, new PopulationFinishedEvent());
}

void ResourceTreeView::_onContextMenu(wxDataViewEvent& ev)
{
    if (!_popupMenu)
    {
        _popupMenu.reset(new wxutil::PopupMenu());

        PopulateContextMenu(*_popupMenu);
    }

    _popupMenu->show(this);
}

bool ResourceTreeView::_testAddToFavourites()
{
    // Adding favourites is allowed for any folder and non-favourite items 
    return IsDirectorySelected() || (GetSelection().IsOk() && !IsFavouriteSelected());
}

bool ResourceTreeView::_testRemoveFromFavourites()
{
    // We can run remove from favourites on any folder or on favourites themselves
    return IsDirectorySelected() || IsFavouriteSelected();
}

void ResourceTreeView::SetFavouriteRecursively(TreeModel::Row& row, bool isFavourite)
{
    if (row[_columns.isFolder].getBool())
    {
        // Enter recursion for this folder
        wxDataViewItemArray children;
        GetModel()->GetChildren(row.getItem(), children);

        for (const wxDataViewItem& child : children)
        {
            TreeModel::Row childRow(child, *GetModel());
            SetFavouriteRecursively(childRow, isFavourite);
        }

        return;
    }

    // Not a folder, set the desired status on this item
    SetFavourite(row, isFavourite);
}

void ResourceTreeView::SetFavourite(TreeModel::Row& row, bool isFavourite)
{
    row[_columns.isFavourite] = isFavourite;
    row[_columns.iconAndName].setAttr(TreeViewItemStyle::Declaration(isFavourite));

    // Keep track of this choice
    if (isFavourite)
    {
        GlobalFavouritesManager().addFavourite(_favouriteTypeName, row[_favouriteKeyColumn]);
    }
    else
    {
        GlobalFavouritesManager().removeFavourite(_favouriteTypeName, row[_favouriteKeyColumn]);
    }

    row.SendItemChanged();
}

void ResourceTreeView::SetFavouriteKeyColumn(const TreeModel::Column& column)
{
    _favouriteKeyColumn = column;
}

void ResourceTreeView::_onSetFavourite(bool isFavourite)
{
    wxDataViewItem item = GetSelection();

    if (!item.IsOk()) return;

    // Grab this item and enter recursion, propagating the favourite status
    TreeModel::Row row(item, *GetModel());

    if (_setFavouritesRecursively)
    {
        SetFavouriteRecursively(row, isFavourite);
    }
    else
    {
        SetFavourite(row, isFavourite);
    }
}

void ResourceTreeView::EnableSetFavouritesRecursively(bool enabled)
{
    _setFavouritesRecursively = enabled;
}

std::string ResourceTreeView::GetResourcePath(const TreeModel::Row& row)
{
    return row[_declPathColumn];
}

void ResourceTreeView::SetDeclPathColumn(const TreeModel::Column& declPathColumn)
{
    _declPathColumn = declPathColumn;
}

std::string ResourceTreeView::GetResourcePathOfSelection()
{
    wxDataViewItem item = GetSelection();

    if (!item.IsOk() || IsDirectorySelected())
    {
        return std::string();
    }

    TreeModel::Row row(item, *GetModel());
    return GetResourcePath(row);
}

void ResourceTreeView::_onCopyResourcePath()
{
    auto resourcePath = GetResourcePathOfSelection();

    if (!resourcePath.empty())
    {
        GlobalClipboard().setString(resourcePath);
    }
}

bool ResourceTreeView::_copyResourcePathEnabled()
{
    return !GetResourcePathOfSelection().empty();
}

bool ResourceTreeView::_copyResourcePathVisible()
{
    return !IsDirectorySelected() && module::GlobalModuleRegistry().moduleExists(MODULE_CLIPBOARD);
}

bool ResourceTreeView::IsDirectorySelected()
{
    wxDataViewItem item = GetSelection();

    if (!item.IsOk()) return false;

    TreeModel::Row row(item, *GetModel());

    return row[_columns.isFolder].getBool();
}

bool ResourceTreeView::IsFavouriteSelected()
{
    wxDataViewItem item = GetSelection();

    if (!item.IsOk()) return false;

    TreeModel::Row row(item, *GetModel());

    return row[_columns.isFavourite].getBool();
}

bool ResourceTreeView::IsTreeModelRowOrAnyChildVisible(TreeModel::Row& row)
{
    // Test the node itself
    if (IsTreeModelRowVisible(row))
    {
        return true; // node is visible
    }

    // The node itself is invisible, but it might still be visible
    // if any of the child nodes is visible, dive into it
    wxDataViewItemArray children;
    _treeStore->GetChildren(row.getItem(), children);

    for (const wxDataViewItem& child : children)
    {
        wxutil::TreeModel::Row childRow(child, *_treeStore);

        // Check the child node (recursively)
        if (IsTreeModelRowOrAnyChildVisible(childRow))
        {
            return true; // found a visible child, this is enough
        }
    }

    // Either we don't have any children, or 
    // all child nodes are invisible, so this node is invisible too
    return false;
}

bool ResourceTreeView::IsTreeModelRowVisible(wxutil::TreeModel::Row& row)
{
    // Check view mode
    if (!IsTreeModelRowVisibleByViewMode(row))
    {
        return false; // done here
    }

    // Check filter, otherwise the node is visible
    return !IsTreeModelRowFiltered(row);
}

bool ResourceTreeView::IsTreeModelRowFiltered(wxutil::TreeModel::Row& row)
{
    return !_filterText.empty() && !TreeModel::RowContainsString(row, _filterText, _colsToSearch, true);
}

bool ResourceTreeView::IsTreeModelRowVisibleByViewMode(wxutil::TreeModel::Row& row)
{
    // In favourites mode, check if this item is marked as favourite
    return _mode == TreeMode::ShowAll || row[_columns.isFavourite].getBool();
}

}
