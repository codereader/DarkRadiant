#pragma once

#include <sigc++/connection.h>
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

    // Subscriptions to the material manager
    sigc::connection _materialCreated;
    sigc::connection _materialRenamed;
    sigc::connection _materialRemoved;

public:
    MaterialTreeView(wxWindow* parent);
    virtual ~MaterialTreeView();

    const TreeColumns& Columns() const;
    
    virtual void SetTreeMode(MaterialTreeView::TreeMode mode) override;

    // Loads all the materials
    virtual void Populate();

private:
    void onMaterialCreated(const std::string& name);
    void onMaterialRenamed(const std::string& oldName, const std::string& newName);
    void onMaterialRemoved(const std::string& name);
};

}
