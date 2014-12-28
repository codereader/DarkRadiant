#pragma once

#include "icommandsystem.h"
#include "imousetoolmanager.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/TreeView.h"
#include <map>

namespace ui
{

class IMouseToolGroup;

class ToolMappingDialog :
    public wxutil::DialogBase
{
public:
    // Treemodel definition
    struct TreeColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        TreeColumns() :
            group(add(wxutil::TreeModel::Column::Integer)),
            toolName(add(wxutil::TreeModel::Column::String)),
            toolDisplayName(add(wxutil::TreeModel::Column::String)),
            mouseButton(add(wxutil::TreeModel::Column::String)),
            modifiers(add(wxutil::TreeModel::Column::String))
        {}

        wxutil::TreeModel::Column group;	        // e.g. 0
        wxutil::TreeModel::Column toolName;	        // e.g. "FreeMoveTool"
        wxutil::TreeModel::Column toolDisplayName;  // e.g. "Toggle Freemove"
        wxutil::TreeModel::Column mouseButton;	    // e.g. "RMB"
        wxutil::TreeModel::Column modifiers;	    // e.g. "CONTROL+SHIFT"
    };

private:
    TreeColumns _columns;

    // Tree store containing all bindings of all groups
    wxutil::TreeModel::Ptr _listStore;

    // Main tree view with model hierarchy
    typedef std::map<IMouseToolGroup::Type, wxutil::TreeView*> TreeViewMap;
    TreeViewMap _treeViews;
    
private:
    void populateWindow();
    void populateListStore();
    wxutil::TreeView* createTreeView(IMouseToolGroup& group);

    void saveToolMapping();

    IMouseToolGroup::Type getGroupType(const wxDataViewItem& item);
    IMouseToolGroup& getGroup(const wxDataViewItem& item);
    MouseToolPtr getTool(const wxDataViewItem& item);

    void onItemActivated(wxDataViewEvent& ev);
    void onResetToDefault(wxCommandEvent& ev);

public:
    // Constructor, creates widgets
    ToolMappingDialog();

    int ShowModal() override;

    static void ShowDialog(const cmd::ArgumentList& args);
};

}
