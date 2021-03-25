#pragma once

#include "wxutil/dataview/ResourceTreeView.h"

namespace ui
{

/**
 * Special tree view for displaying the available materials
 */
class MaterialTreeView :
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

public:
    MaterialTreeView(wxWindow* parent);

    const TreeColumns& Columns() const;
    
    virtual void SetTreeMode(MaterialTreeView::TreeMode mode) override;

    // Loads all the materials
    virtual void Populate();
};

}
