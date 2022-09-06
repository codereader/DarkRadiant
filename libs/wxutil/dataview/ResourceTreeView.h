#pragma once

#include "idecltypes.h"
#include "TreeView.h"
#include "TreeModel.h"
#include "TreeModelFilter.h"
#include "IResourceTreePopulator.h"
#include "../menu/PopupMenu.h"
#include "wxutil/Icon.h"

namespace wxutil
{

/**
 * A specialised tree view for display resources like materials,
 * prefabs, sound shaders and the like. It defines the default Column set
 * to use for any treestore that is associated to this view - this set can
 * be derived from and extended to display more sophisticated models.
 * 
 * This tree control supports two "modes", one showing all elements in the tree,
 * and the other showing favourites only: see getTreeMode()/setTreeMode().
 * 
 * It ships with a context menu that can be customised by subclasses,
 * to extend the default actions aimed at managing favourites.
 */
class ResourceTreeView :
    public TreeView
{
public:
    // The base structure defining a few needed default column.
    // Subclasses may derive from this struct to expand it
    struct Columns :
        public TreeModel::ColumnRecord
    {
        Columns() :
            iconAndName(add(TreeModel::Column::IconText)),
            leafName(add(TreeModel::Column::String)),
            fullName(add(TreeModel::Column::String)),
            isFolder(add(TreeModel::Column::Boolean)),
            isFavourite(add(TreeModel::Column::Boolean))
        {}

        TreeModel::Column iconAndName;
        TreeModel::Column leafName; // name without parent folders
        TreeModel::Column fullName; // name including parent folders
        TreeModel::Column isFolder;
        TreeModel::Column isFavourite;
    };

    // Filter modes used by this tree view
    enum class TreeMode
    {
        ShowAll,
        ShowFavourites,
    };

    // Event type emitted when the tree view is ready
    class PopulationFinishedEvent :
        public wxEvent
    {
    public:
        PopulationFinishedEvent(int id = 0);
        PopulationFinishedEvent(const PopulationFinishedEvent& event);

        wxEvent* Clone() const;
    };

private:
    // Context menu
    PopupMenuPtr _popupMenu;

    const Columns& _columns;

    TreeMode _mode;

    TreeModel::Ptr _treeStore;
    TreeModelFilter::Ptr _treeModelFilter;
    wxDataViewItem _emptyFavouritesLabel;
    wxDataViewItem _progressItem;
    Icon _progressIcon;

    // The currently active populator object
    IResourceTreePopulator::Ptr _populator;

    bool _expandTopLevelItemsAfterPopulation;
    std::string _elementToSelectAfterPopulation;
    const TreeModel::Column* _columnToSelectAfterPopulation;

    std::vector<ui::IMenuItemPtr> _customMenuItems;

    // Typename used to store favourites. Favourites disabled if empty.
    std::string _favouriteTypeName;
    bool _setFavouritesRecursively;

    wxString _filterText;

    // The column that is hosting the declaration path (used by e.g. "copy to clipboard")
    TreeModel::Column _declPathColumn;
    TreeModel::Column _favouriteKeyColumn;

public:
    ResourceTreeView(wxWindow* parent, const Columns& columns, long style = wxDV_SINGLE);
    ResourceTreeView(wxWindow* parent, const TreeModel::Ptr& model, const Columns& columns, long style = wxDV_SINGLE);

    virtual ~ResourceTreeView();

    // Returns a reference to the model we're using
    virtual const TreeModel::Ptr& GetTreeModel();
    virtual void SetTreeModel(const TreeModel::Ptr& treeModel);

    virtual TreeMode GetTreeMode() const;
    virtual void SetTreeMode(TreeMode mode);

    // Sets the string filter to apply to the currently visible tree
    // this string will match against the default iconAndName column,
    // all rows not containing the string will be hidden.
    // Filtering happens case-insensitively.
    // As feedback this method returns true if the filter has any matches.
    virtual bool SetFilterText(const wxString& filterText);

    // Removes the string filter
    virtual void ClearFilterText();

    // Returns the full name of the selection (or an empty string)
    virtual std::string GetSelectedFullname();
    virtual void SetSelectedFullname(const std::string& fullName);

    virtual std::string GetSelectedElement(const TreeModel::Column& column);
    virtual void SetSelectedElement(const std::string& value, const TreeModel::Column& column);

    // Set the column containing the resource path used to define the game-compatible
    // declaration path, also used by the "Copy resource path" context menu item
    virtual void SetDeclPathColumn(const TreeModel::Column& declPathColumn);

    virtual void Clear();

    // Enable favourite management for the given favourite type name
    virtual void EnableFavouriteManagement(const std::string& typeName);
    // Disable favourite management features
    virtual void DisableFavouriteManagement();

    // Whether add/remove favourites is operating recursively
    virtual void EnableSetFavouritesRecursively(bool enabled);

    virtual bool IsDirectorySelected();
    virtual bool IsFavouriteSelected();

    // Populate this tree using the given populator object
    virtual void Populate(const IResourceTreePopulator::Ptr& populator);

    void SetExpandTopLevelItemsAfterPopulation(bool expand);

    // Add a custom menu item to this control's popup menu (will be added at the top)
    // Client code that derive from this class can use the protected PopulateContextMenu
    // hook as an alternative to this method.
    void AddCustomMenuItem(const ui::IMenuItemPtr& item);

    // Returns true if there is a first filter match
    virtual bool JumpToFirstFilterMatch();
    virtual void JumpToNextFilterMatch();
    virtual void JumpToPrevFilterMatch();

protected:
    virtual void PopulateContextMenu(wxutil::PopupMenu& popupMenu);

    virtual void SetFavourite(TreeModel::Row& row, bool isFavourite);
    virtual void SetFavouriteRecursively(TreeModel::Row& row, bool isFavourite);

    // Defines the column that is used to retrieve the key when adding/removing favourites
    virtual void SetFavouriteKeyColumn(const TreeModel::Column& column);

    virtual void SetupTreeModelFilter();

    virtual bool IsTreeModelRowVisible(TreeModel::Row& row);

    virtual void UpdateTreeVisibility();

    // Get the resource path for the given item. Determines the availabilty
    // and functionality of the "Copy resource path" context menu item
    // The default implementation returns the value of the "fullName" column.
    virtual std::string GetResourcePath(const TreeModel::Row& row);

    // Retrieve the resource path of the currently selected item
    virtual std::string GetResourcePathOfSelection();

private:
    // Recursive visibility test used by the TreeModelFilterFunction
    bool IsTreeModelRowOrAnyChildVisible(TreeModel::Row& row);

    // Returns true if the given row is visible according 
    // to the current view mode (show favourites vs. show all)
    bool IsTreeModelRowVisibleByViewMode(TreeModel::Row& row);
    
    // Returns true if the given row is filtered by an active filter text
    bool IsTreeModelRowFiltered(wxutil::TreeModel::Row& row);

    void _onContextMenu(wxDataViewEvent& ev);
    void _onTreeStorePopulationProgress(TreeModel::PopulationProgressEvent& ev);
    void _onTreeStorePopulationFinished(TreeModel::PopulationFinishedEvent& ev);

    bool _testAddToFavourites();
    bool _testRemoveFromFavourites();
    void _onSetFavourite(bool isFavourite);
    void _onCopyResourcePath();
    bool _copyResourcePathEnabled();
    bool _copyResourcePathVisible();
};

// Emitted when the tree view is done populating
wxDECLARE_EVENT(EV_TREEVIEW_POPULATION_FINISHED, ResourceTreeView::PopulationFinishedEvent);
// Emitted when the tree view cleared its filter text on its own
wxDECLARE_EVENT(EV_TREEVIEW_FILTERTEXT_CLEARED, wxCommandEvent);

}
