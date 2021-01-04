#pragma once

#include "wxutil/dataview/ResourceTreeView.h"

namespace ui
{

class ModelTreeView :
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
            isSkin(add(wxutil::TreeModel::Column::Boolean))
        {}

        // iconAndName column contains the filename, e.g. "chair1.lwo"
        // fullPath column contains the VFS path to the model plus skin info, e.g. "models/darkmod/props/chair1.lwo[/skinName]"
        wxutil::TreeModel::Column skin;		// e.g. "chair1_brown_wood", or "" for no skin
        wxutil::TreeModel::Column modelPath;// e.g. "models/darkmod/props/chair1.lwo"
        wxutil::TreeModel::Column isSkin;	// TRUE if this is a skin entry, FALSE if actual model or folder
    };

private:
    bool _showSkins;
    TreeColumns _columns;

    wxDataViewItem _progressItem;
    wxIcon _modelIcon;

public:
    ModelTreeView(wxWindow* parent);

    // Start populating the model tree in the background
    void Populate();

    void SetShowSkins(bool showSkins);

    std::string GetSelectedModelPath();
    std::string GetSelectedSkin();

protected:
    bool IsTreeModelRowVisible(wxutil::TreeModel::Row& row) override;

private:
    std::string GetColumnValue(const wxutil::TreeModel::Column& column);
};

}
