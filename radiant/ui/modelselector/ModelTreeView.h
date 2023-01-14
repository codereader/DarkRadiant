#pragma once

#include "wxutil/dataview/ResourceTreeView.h"

namespace ui
{

class ModelTreeView final :
    public wxutil::ResourceTreeView
{
public:
    // Treemodel definition
    struct TreeColumns :
        public wxutil::ResourceTreeView::Columns
    {
        TreeColumns() :
            modelPath(add(wxutil::TreeModel::Column::String)),
            skin(add(wxutil::TreeModel::Column::String)),
            isSkin(add(wxutil::TreeModel::Column::Boolean)),
            isModelDefFolder(add(wxutil::TreeModel::Column::Boolean))
        {}

        // iconAndName column contains the filename, e.g. "chair1.lwo"
        // fullPath column contains the VFS path to the model plus skin info, e.g. "models/darkmod/props/chair1.lwo[/skinName]"
        wxutil::TreeModel::Column skin;		// e.g. "chair1_brown_wood", or "" for no skin
        wxutil::TreeModel::Column modelPath;// e.g. "models/darkmod/props/chair1.lwo"
        wxutil::TreeModel::Column isSkin;	// TRUE if this is a skin entry, FALSE if actual model or folder
        wxutil::TreeModel::Column isModelDefFolder;	// TRUE if this is the model def folder, which should sort last
    };

private:
    bool _showSkins;

    wxDataViewItem _progressItem;

public:
    ModelTreeView(wxWindow* parent);

    // Start populating the model tree in the background
    void Populate();

    void SetShowSkins(bool showSkins);

    std::string GetSelectedModelPath();
    std::string GetSelectedSkin();

    // Highlights the given skin of the currently selected model
    // Does nothing if there's no such skin for the current model
    // or if there's no model at all. Only functional if _showSkins is enabled.
    void SetSelectedSkin(const std::string& skin);

    void CollapseModelDefsFolder();

protected:
    bool IsTreeModelRowVisible(wxutil::TreeModel::Row& row) override;

    const TreeColumns& Columns() const;

    // Override the resource path locator of the base class since we store
    // skin paths and model paths in different columns
    std::string GetResourcePath(const wxutil::TreeModel::Row& row) override;

private:
    std::string GetColumnValue(const wxutil::TreeModel::Column& column);
    void showDefinition();
    bool testShowDefinition();
};

}
