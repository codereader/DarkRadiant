#pragma once

#include "wxutil/dataview/ResourceTreeView.h"
#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/dataview/TreeModelFilter.h"

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

private:
    // Populates the Media Browser in its own thread
    std::unique_ptr<wxutil::ThreadedResourceTreePopulator> _populator;

    // false, if the tree is not yet initialised.
    bool _isPopulated;

    MediaBrowserTreeView::TreeColumns _columns;

public:
    MediaBrowserTreeView(wxWindow* parent);

    const TreeColumns& getColumns() const;

    void setSelection(const std::string& fullName) override;

    // Clear all items, stop any populator thread
    void clear() override;

    void setTreeMode(TreeMode mode) override;

    // Populates the treeview
    void populate();

protected:
    void populateContextMenu(wxutil::PopupMenu& popupMenu) override;

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
};

}
