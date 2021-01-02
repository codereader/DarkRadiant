#pragma once

#include "wxutil/dataview/ResourceTreeView.h"
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
    MediaBrowserTreeView::TreeColumns _columns;

public:
    MediaBrowserTreeView(wxWindow* parent);

    const TreeColumns& getColumns() const;

    void setTreeMode(TreeMode mode) override;

    // Loads all the materials
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
};

}
