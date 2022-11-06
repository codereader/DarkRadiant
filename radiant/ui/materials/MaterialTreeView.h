#pragma once

#include <sigc++/connection.h>
#include "wxutil/dataview/DeclarationTreeView.h"

namespace ui
{

/**
 * Special tree view for displaying the available materials
 */
class MaterialTreeView :
    public wxutil::DeclarationTreeView
{
public:
    struct TreeColumns :
        public DeclarationTreeView::Columns
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

    // Returns the name of the texture folder without any "Other Materials" prefix
    // Returns an empty string if no folder is selected
    std::string GetSelectedTextureFolderName();

    // Loads all the materials
    virtual void Populate();

private:
    void onMaterialCreated(const std::string& name);
    void onMaterialRenamed(const std::string& oldName, const std::string& newName);
    void onMaterialRemoved(const std::string& name);
};

}
