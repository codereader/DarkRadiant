#pragma once

#include "imap.h"

#include <sigc++/connection.h>

#include "wxutil/DockablePanel.h"
#include "wxutil/dataview/TreeModel.h"
#include "wxutil/event/SingleIdleCallback.h"
#include "wxutil/menu/PopupMenu.h"

namespace wxutil
{
    class TreeView;
}

class wxButton;

namespace ui
{

class SelectionGroupPanel :
    public wxutil::DockablePanel,
    public wxutil::SingleIdleCallback
{
private:
    struct TreeColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        TreeColumns() :
            groupId(add(wxutil::TreeModel::Column::Integer)),
            name(add(wxutil::TreeModel::Column::String)),
            memberCount(add(wxutil::TreeModel::Column::String)),
            isGroup(add(wxutil::TreeModel::Column::Boolean)),
            node(add(wxutil::TreeModel::Column::Pointer))
        {}

        wxutil::TreeModel::Column groupId;
        wxutil::TreeModel::Column name;
        wxutil::TreeModel::Column memberCount;
        wxutil::TreeModel::Column isGroup;
        wxutil::TreeModel::Column node;
    };

    wxutil::TreeView* _treeView;
    TreeColumns _columns;
    wxutil::TreeModel::Ptr _treeStore;

    wxButton* _removeFromGroupButton;
    wxButton* _deleteButton;

    bool _refreshOnIdle;
    bool _callbackActive;

    sigc::connection _mapEventSignal;
    sigc::connection _undoEventSignal;

    wxutil::PopupMenuPtr _popupMenu;

public:
    SelectionGroupPanel(wxWindow* parent);
    ~SelectionGroupPanel() override;

protected:
    void onIdle() override;

    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectListeners();
    void disconnectListeners();
    void connectToMapRoot();
    void disconnectFromMapRoot();

    void queueRefresh();
    void refresh();

    void populateWindow();
    void createButtons();
    void createPopupMenu();

    void onMapEvent(IMap::MapEvent ev);
    void onItemActivated(wxDataViewEvent& ev);
    void onItemSelected(wxDataViewEvent& ev);

    void updateButtonSensitivity();

    void removeSelectedFromGroup();
    void deleteSelectedGroup();

    std::size_t getSelectedGroupId();
    bool hasSelectedMembers();
    bool hasMapRoot();
};

}
