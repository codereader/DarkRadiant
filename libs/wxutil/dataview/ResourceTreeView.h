#pragma once

#include "idecltypes.h"
#include "TreeView.h"
#include "TreeModel.h"
#include "TreeModelFilter.h"
#include "IResourceTreePopulator.h"
#include "../menu/PopupMenu.h"

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
    wxIcon _progressIcon;

    // The currently active populator object
    IResourceTreePopulator::Ptr _populator;

    bool _expandTopLevelItemsAfterPopulation;
    std::string _fullNameToSelectAfterPopulation;

    std::vector<ui::IMenuItemPtr> _customMenuItems;

    decl::Type _declType;

public:
    ResourceTreeView(wxWindow* parent, const Columns& columns, long style = wxDV_SINGLE);
    ResourceTreeView(wxWindow* parent, const TreeModel::Ptr& model, const Columns& columns, long style = wxDV_SINGLE);

    virtual ~ResourceTreeView();

    // Returns a reference to the model we're using
    virtual const TreeModel::Ptr& GetTreeModel();
    virtual void SetTreeModel(const TreeModel::Ptr& treeModel);

    virtual TreeMode GetTreeMode() const;
    virtual void SetTreeMode(TreeMode mode);

    // Returns the full name of the selection (or an empty string)
    virtual std::string GetSelectedFullname();
    virtual void SetSelectedFullname(const std::string& fullName);

    virtual void Clear();

    // Enable favourite management for the given declaration type
    virtual void EnableFavouriteManagement(decl::Type declType);
    // Disable favourite management features
    virtual void DisableFavouriteManagement();

    virtual bool IsDirectorySelected();
    virtual bool IsFavouriteSelected();

    // Populate this tree using the given populator object
    virtual void Populate(const IResourceTreePopulator::Ptr& populator);

    void SetExpandTopLevelItemsAfterPopulation(bool expand);

    // Add a custom menu item to this control's popup menu (will be added at the top)
    // Client code that derive from this class can use the protected PopulateContextMenu
    // hook as an alternative to this method.
    void AddCustomMenuItem(const ui::IMenuItemPtr& item);

protected:
    virtual void PopulateContextMenu(wxutil::PopupMenu& popupMenu);

    virtual void SetFavouriteRecursively(TreeModel::Row& row, bool isFavourite);

    virtual void SetupTreeModelFilter();

    virtual bool IsTreeModelRowVisible(TreeModel::Row& row);

private:
    void _onContextMenu(wxDataViewEvent& ev);
    void _onTreeStorePopulationProgress(TreeModel::PopulationProgressEvent& ev);
    void _onTreeStorePopulationFinished(TreeModel::PopulationFinishedEvent& ev);

    bool _testAddToFavourites();
    bool _testRemoveFromFavourites();
    void _onSetFavourite(bool isFavourite);
};

// Emitted when the tree view is done populating
wxDECLARE_EVENT(EV_TREEVIEW_POPULATION_FINISHED, ResourceTreeView::PopulationFinishedEvent);

}
