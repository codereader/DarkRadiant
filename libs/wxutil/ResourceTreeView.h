#pragma once

#include "TreeView.h"
#include "TreeModel.h"
#include "menu/PopupMenu.h"

namespace wxutil
{

/**
 * A specialised tree view for display resources like materials,
 * prefabs, sound shaders and the like.
 * It ships with a context menu that offers default actions for
 * managing favourites.
 */
class ResourceTreeView :
    public TreeView
{
public:
    // The base structure defining a few needed default column.
    // Subclasses may derive from this struct to expand it
    struct Columns :
        public wxutil::TreeModel::ColumnRecord
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

private:
    // Context menu
    PopupMenuPtr _popupMenu;

    const Columns& _columns;

protected:
    virtual void populateContextMenu(wxutil::PopupMenu& popupMenu);

    virtual void setFavouriteRecursively(TreeModel::Row& row, bool isFavourite);

public:
    ResourceTreeView(wxWindow* parent, const Columns& columns, long style = wxDV_SINGLE);
    ResourceTreeView(wxWindow* parent, wxDataViewModel* model, const Columns& columns, long style = wxDV_SINGLE);

    // Returns the full name of the selection (or an empty string)
    std::string getSelection();

    bool isDirectorySelected();
    bool isFavouriteSelected();

private:
    void _onContextMenu(wxDataViewEvent& ev);

    bool _testAddToFavourites();
    bool _testRemoveFromFavourites();
    void _onSetFavourite(bool isFavourite);
};

}
