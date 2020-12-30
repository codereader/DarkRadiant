#pragma once

#include "wxutil/ResourceTreeView.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/TreeModelFilter.h"

namespace ui
{

class MediaBrowserTreeView :
    public wxutil::ResourceTreeView
{
public:
    struct TreeColumns :
        public wxutil::ResourceTreeView::Columns
    {
        // We just need one additional column to store the "is other materials folder" flag
        TreeColumns() :
            isOtherMaterialsFolder(add(wxutil::TreeModel::Column::Boolean))
        {}

        wxutil::TreeModel::Column isOtherMaterialsFolder;
    };

    enum class TreeMode
    {
        ShowAll,
        ShowFavourites,
    };

private:
    // Populates the Media Browser in its own thread
    class Populator;
    std::unique_ptr<Populator> _populator;

    // false, if the tree is not yet initialised.
    bool _isPopulated;

    TreeMode _mode;

    MediaBrowserTreeView::TreeColumns _columns;
    wxutil::TreeModel::Ptr _treeStore;
    wxutil::TreeModelFilter::Ptr _treeModelFilter;
    wxDataViewItem _emptyFavouritesLabel;

public:
    MediaBrowserTreeView(wxWindow* parent);

    const TreeColumns& getColumns() const;

    TreeMode getTreeMode() const;
    void setTreeMode(TreeMode mode);

    void setSelection(const std::string& fullName);

    // Clear all items, stop any populator thread
    void clear();

    // Populates the treeview
    void populate();

protected:
    void setFavouriteRecursively(wxutil::TreeModel::Row& row, bool isFavourite) override;
    void populateContextMenu(wxutil::PopupMenu& popupMenu) override;

    // Evaulation function for item visibility
    bool treeModelFilterFunc(wxutil::TreeModel::Row& row);

private:
    bool _testSingleTexSel();
    bool _testLoadInTexView();
    void _onApplyToSel();
    void _onShowShaderDefinition();
    void _onLoadInTexView();
    void _onSelectItems(bool select);
    void _onTreeViewItemActivated(wxDataViewEvent& ev);
    void _onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev);
    void _onExpose(wxPaintEvent& ev);

    void handleTreeModeChanged();
    void setupTreeViewAndFilter();
};

}
