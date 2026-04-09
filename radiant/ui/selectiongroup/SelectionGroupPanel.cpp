#include "SelectionGroupPanel.h"

#include "i18n.h"
#include "imap.h"
#include "iundo.h"
#include "iselectable.h"
#include "iselectiongroup.h"
#include "scene/Node.h"

#include "wxutil/dataview/TreeView.h"
#include "util/ScopedBoolLock.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/wupdlock.h>

#include <fmt/format.h>

namespace ui
{

SelectionGroupPanel::SelectionGroupPanel(wxWindow* parent) :
    DockablePanel(parent),
    _treeView(nullptr),
    _removeFromGroupButton(nullptr),
    _deleteButton(nullptr),
    _refreshOnIdle(false),
    _callbackActive(false)
{
    populateWindow();
}

SelectionGroupPanel::~SelectionGroupPanel()
{
    if (_treeView)
    {
        _treeView->Unbind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &SelectionGroupPanel::onItemActivated, this);
        _treeView->Unbind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SelectionGroupPanel::onItemSelected, this);
    }

    if (panelIsActive())
    {
        disconnectListeners();
    }
}

void SelectionGroupPanel::onPanelActivated()
{
    connectListeners();
    queueRefresh();
}

void SelectionGroupPanel::onPanelDeactivated()
{
    disconnectListeners();
    cancelCallbacks();
}

void SelectionGroupPanel::connectListeners()
{
    _mapEventSignal = GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(this, &SelectionGroupPanel::onMapEvent));

    connectToMapRoot();
}

void SelectionGroupPanel::disconnectListeners()
{
    _mapEventSignal.disconnect();
    disconnectFromMapRoot();
    _refreshOnIdle = false;
}

void SelectionGroupPanel::connectToMapRoot()
{
    disconnectFromMapRoot();

    if (GlobalMapModule().getRoot())
    {
        _undoEventSignal = GlobalUndoSystem().signal_undoEvent().connect(
            [this](IUndoSystem::EventType, const std::string&) { queueRefresh(); });
    }
}

void SelectionGroupPanel::disconnectFromMapRoot()
{
    _undoEventSignal.disconnect();
}

void SelectionGroupPanel::populateWindow()
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    auto* vbox = new wxBoxSizer(wxVERTICAL);
    GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

    _treeStore = new wxutil::TreeModel(_columns);

    _treeView = wxutil::TreeView::CreateWithModel(this, _treeStore.get(), wxDV_MULTIPLE);

    _treeView->AppendTextColumn(_("Group"), _columns.name.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    _treeView->AppendTextColumn(_("Members"), _columns.memberCount.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

    _treeView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &SelectionGroupPanel::onItemActivated, this);
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SelectionGroupPanel::onItemSelected, this);

    vbox->Add(_treeView, 1, wxEXPAND | wxBOTTOM, 6);

    createButtons();
    createPopupMenu();
}

void SelectionGroupPanel::createButtons()
{
    auto* buttonBox = new wxBoxSizer(wxHORIZONTAL);

    _removeFromGroupButton = new wxButton(this, wxID_ANY, _("Remove from Group"));
    _removeFromGroupButton->Bind(wxEVT_BUTTON, [this](auto&) { removeSelectedFromGroup(); });
    _removeFromGroupButton->SetToolTip(_("Remove the selected members from their group"));
    _removeFromGroupButton->Enable(false);

    _deleteButton = new wxButton(this, wxID_ANY, _("Delete Group"));
    _deleteButton->Bind(wxEVT_BUTTON, [this](auto&) { deleteSelectedGroup(); });
    _deleteButton->SetToolTip(_("Delete the entire selected group"));
    _deleteButton->Enable(false);

    buttonBox->Add(_removeFromGroupButton, 1, wxEXPAND | wxRIGHT, 6);
    buttonBox->Add(_deleteButton, 1, wxEXPAND, 6);

    GetSizer()->Add(buttonBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
}

void SelectionGroupPanel::createPopupMenu()
{
    _popupMenu = std::make_shared<wxutil::PopupMenu>();

    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Select group members")),
        [this]() {
            auto id = getSelectedGroupId();
            if (id == 0 || !hasMapRoot()) return;
            auto& mgr = GlobalMapModule().getRoot()->getSelectionGroupManager();
            auto group = mgr.getSelectionGroup(id);
            if (group) group->setSelected(true);
        },
        [this]() { return getSelectedGroupId() != 0; }
    );

    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Deselect group members")),
        [this]() {
            auto id = getSelectedGroupId();
            if (id == 0 || !hasMapRoot()) return;
            auto& mgr = GlobalMapModule().getRoot()->getSelectionGroupManager();
            auto group = mgr.getSelectionGroup(id);
            if (group) group->setSelected(false);
        },
        [this]() { return getSelectedGroupId() != 0; }
    );

    _popupMenu->addSeparator();

    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Remove from group")),
        [this]() { removeSelectedFromGroup(); },
        [this]() { return hasSelectedMembers(); }
    );

    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Delete group")),
        [this]() { deleteSelectedGroup(); },
        [this]() { return getSelectedGroupId() != 0; }
    );

    _treeView->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
        [this](auto&) { _popupMenu->show(this); });
}

void SelectionGroupPanel::queueRefresh()
{
    _refreshOnIdle = true;
    requestIdleCallback();
}

void SelectionGroupPanel::onIdle()
{
    if (_refreshOnIdle)
    {
        _refreshOnIdle = false;
        refresh();
    }
}

void SelectionGroupPanel::refresh()
{
    if (!hasMapRoot())
    {
        _treeStore->Clear();
        updateButtonSensitivity();
        return;
    }

    util::ScopedBoolLock lock(_callbackActive);
    wxWindowUpdateLocker freezer(_treeView);

    _treeStore->Clear();

    auto& mgr = GlobalMapModule().getRoot()->getSelectionGroupManager();

    mgr.foreachSelectionGroup([&](selection::ISelectionGroup& group)
    {
        auto groupRow = _treeStore->AddItem();

        std::string displayName = group.getName().empty()
            ? fmt::format(_("Group {0}"), group.getId())
            : group.getName();

        groupRow[_columns.groupId] = static_cast<int>(group.getId());
        groupRow[_columns.name] = displayName;
        groupRow[_columns.memberCount] = std::to_string(group.size());
        groupRow[_columns.isGroup] = true;
        groupRow[_columns.node] = wxVariant(static_cast<void*>(nullptr));

        groupRow.SendItemAdded();

        group.foreachNode([&](const scene::INodePtr& node)
        {
            auto childRow = _treeStore->AddItemUnderParent(groupRow.getItem());

            childRow[_columns.groupId] = static_cast<int>(group.getId());
            childRow[_columns.name] = node->name();
            childRow[_columns.memberCount] = std::string();
            childRow[_columns.isGroup] = false;
            childRow[_columns.node] = wxVariant(node.get());

            childRow.SendItemAdded();
        });
    });

    updateButtonSensitivity();
}

void SelectionGroupPanel::onMapEvent(IMap::MapEvent ev)
{
    if (ev == IMap::MapLoaded)
    {
        connectToMapRoot();
        queueRefresh();
    }
    else if (ev == IMap::MapUnloading)
    {
        disconnectFromMapRoot();
        queueRefresh();
    }
}

void SelectionGroupPanel::onItemActivated(wxDataViewEvent& ev)
{
    if (_callbackActive) return;

    auto item = ev.GetItem();
    if (!item.IsOk() || !hasMapRoot()) return;

    wxutil::TreeModel::Row row(item, *_treeStore);

    if (row[_columns.isGroup].getBool())
    {
        auto id = static_cast<std::size_t>(row[_columns.groupId].getInteger());
        auto& mgr = GlobalMapModule().getRoot()->getSelectionGroupManager();
        auto group = mgr.getSelectionGroup(id);
        if (group) group->setSelected(true);
    }
    else
    {
        auto* nodePtr = static_cast<scene::INode*>(row[_columns.node].getPointer());
        if (!nodePtr) return;

        auto* selectable = dynamic_cast<ISelectable*>(nodePtr);
        if (selectable) selectable->setSelected(true);
    }
}

void SelectionGroupPanel::onItemSelected(wxDataViewEvent& ev)
{
    updateButtonSensitivity();
}

void SelectionGroupPanel::updateButtonSensitivity()
{
    _removeFromGroupButton->Enable(hasSelectedMembers());
    _deleteButton->Enable(getSelectedGroupId() != 0);
}

void SelectionGroupPanel::removeSelectedFromGroup()
{
    if (!hasMapRoot()) return;

    wxDataViewItemArray selection;
    _treeView->GetSelections(selection);
    if (selection.empty()) return;

    auto& mgr = GlobalMapModule().getRoot()->getSelectionGroupManager();

    for (const auto& item : selection)
    {
        if (!item.IsOk()) continue;

        wxutil::TreeModel::Row row(item, *_treeStore);

        if (row[_columns.isGroup].getBool()) continue;

        auto groupId = static_cast<std::size_t>(row[_columns.groupId].getInteger());
        auto* nodePtr = static_cast<scene::INode*>(row[_columns.node].getPointer());
        if (!nodePtr) continue;

        auto group = mgr.getSelectionGroup(groupId);
        if (!group) continue;

        auto* sceneNode = dynamic_cast<scene::Node*>(nodePtr);
        if (sceneNode)
        {
            group->removeNode(sceneNode->shared_from_this());
        }
    }

    queueRefresh();
}

void SelectionGroupPanel::deleteSelectedGroup()
{
    auto id = getSelectedGroupId();
    if (id == 0 || !hasMapRoot()) return;

    auto& mgr = GlobalMapModule().getRoot()->getSelectionGroupManager();
    mgr.deleteSelectionGroup(id);

    queueRefresh();
}

std::size_t SelectionGroupPanel::getSelectedGroupId()
{
    wxDataViewItemArray selection;
    _treeView->GetSelections(selection);

    for (const auto& item : selection)
    {
        if (!item.IsOk()) continue;

        wxutil::TreeModel::Row row(item, *_treeStore);

        if (row[_columns.isGroup].getBool())
        {
            return static_cast<std::size_t>(row[_columns.groupId].getInteger());
        }
    }

    return 0;
}

bool SelectionGroupPanel::hasSelectedMembers()
{
    wxDataViewItemArray selection;
    _treeView->GetSelections(selection);

    for (const auto& item : selection)
    {
        if (!item.IsOk()) continue;

        wxutil::TreeModel::Row row(item, *_treeStore);

        if (!row[_columns.isGroup].getBool())
        {
            return true;
        }
    }

    return false;
}

bool SelectionGroupPanel::hasMapRoot()
{
    return GlobalMapModule().getRoot() != nullptr;
}

}
