#pragma once

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

private:
    // Context menu
    PopupMenuPtr _popupMenu;

    const Columns& _columns;

    TreeMode _mode;

    TreeModel::Ptr _treeStore;
    TreeModelFilter::Ptr _treeModelFilter;
    wxDataViewItem _emptyFavouritesLabel;

    // The currently active populator object
    IResourceTreePopulator::Ptr _populator;

    bool _expandTopLevelItemsAfterPopulation;
    std::string _itemToSelectAfterPopulation;

public:
    ResourceTreeView(wxWindow* parent, const Columns& columns, long style = wxDV_SINGLE);
    ResourceTreeView(wxWindow* parent, const TreeModel::Ptr& model, const Columns& columns, long style = wxDV_SINGLE);

    virtual ~ResourceTreeView();

    // Returns a reference to the model we're using
    virtual const TreeModel::Ptr& getTreeModel();
    virtual void setTreeModel(const TreeModel::Ptr& treeModel);

    virtual TreeMode getTreeMode() const;
    virtual void setTreeMode(TreeMode mode);

    // Returns the full name of the selection (or an empty string)
    virtual std::string getSelection();
    virtual void setSelection(const std::string& fullName);

    virtual void clear();

    virtual bool isDirectorySelected();
    virtual bool isFavouriteSelected();

    // Populate this tree using the given populator object
    virtual void populate(const IResourceTreePopulator::Ptr& populator);

    void setExpandTopLevelItemsAfterPopulation(bool expand);

protected:
    virtual void populateContextMenu(wxutil::PopupMenu& popupMenu);

    virtual void setFavouriteRecursively(TreeModel::Row& row, bool isFavourite);

    virtual void setupTreeModelFilter();

private:
    void _onContextMenu(wxDataViewEvent& ev);
    void _onTreeStorePopulationFinished(TreeModel::PopulationFinishedEvent& ev);

    bool _testAddToFavourites();
    bool _testRemoveFromFavourites();
    void _onSetFavourite(bool isFavourite);

    // Evaluation function for item visibility
    bool treeModelFilterFunc(TreeModel::Row& row);
};

}
