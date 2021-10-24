#include "MergeControlDialog.h"

#include "i18n.h"
#include "itextstream.h"
#include "ui/imainframe.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "iundo.h"
#include "imap.h"
#include "wxutil/PathEntry.h"
#include "wxutil/Bitmap.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/dialog/MessageBox.h"
#include "scenelib.h"
#include "string/convert.h"
#include "os/path.h"
#include "fmt/format.h"
#include "scene/merge/MergeAction.h"
#include "scene/merge/MergeLib.h"

#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/tglbtn.h>

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Merge Maps");

    const std::string RKEY_ROOT = "user/ui/mergeControlDialog/";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

namespace
{

// Column setup for the list store
struct Columns :
    public wxutil::TreeModel::ColumnRecord
{
    Columns() :
        description(add(wxutil::TreeModel::Column::String))
    {}

    wxutil::TreeModel::Column description;
};

const Columns& COLUMNS()
{
    static const Columns _instance;
    return _instance;
}

}

MergeControlDialog::MergeControlDialog() :
    TransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true),
    _updateNeeded(false),
    _numUnresolvedConflicts(0)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));
    GetSizer()->Add(loadNamedPanel(this, "MergeControlDialogMainPanel"), 1, wxEXPAND);

    convertTextCtrlToPathEntry("MergeMapFilename");
    convertTextCtrlToPathEntry("BaseMapFilename");

    auto* mergeSourceEntry = findNamedObject<wxutil::PathEntry>(this, "MergeMapFilename");
    mergeSourceEntry->Bind(wxutil::EV_PATH_ENTRY_CHANGED, &MergeControlDialog::onMergeSourceChanged, this);

    auto* baseMapEntry = findNamedObject<wxutil::PathEntry>(this, "BaseMapFilename");
    baseMapEntry->Bind(wxutil::EV_PATH_ENTRY_CHANGED, &MergeControlDialog::onMergeSourceChanged, this);

    auto* abortMergeButton = findNamedObject<wxButton>(this, "AbortMergeButton");
    abortMergeButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onAbortMerge, this);

    auto* loadAndCompareButton = findNamedObject<wxButton>(this, "LoadAndCompareButton");
    loadAndCompareButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onLoadAndCompare, this);

    auto* finishButton = findNamedObject<wxButton>(this, "FinishMergeButton");
    finishButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onFinishMerge, this);

    auto* resolveAcceptButton = findNamedObject<wxButton>(this, "ResolveAcceptButton");
    resolveAcceptButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onResolveAccept, this);

    auto* resolveRejectButton = findNamedObject<wxButton>(this, "ResolveRejectButton");
    resolveRejectButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onResolveReject, this);

    auto* resolveKeepBothButton = findNamedObject<wxButton>(this, "ResolveKeepBothButton");
    resolveKeepBothButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onResolveKeepBoth, this);

    auto* jumpToNextConflict = findNamedObject<wxButton>(this, "JumpToNextConflictButton");
    jumpToNextConflict->Bind(wxEVT_BUTTON, &MergeControlDialog::onJumpToNextConflict, this);

    findNamedObject<wxCheckBox>(this, "KeepSelectionGroupsIntact")->SetValue(true);
    findNamedObject<wxCheckBox>(this, "MergeLayers")->SetValue(true);

    auto twoWayButton = findNamedObject<wxToggleButton>(this, "TwoWayMode");
    twoWayButton->SetBitmap(wxutil::GetLocalBitmap("two_way_merge.png"));
    twoWayButton->Bind(wxEVT_TOGGLEBUTTON, &MergeControlDialog::onMergeModeChanged, this);

    auto threeWayButton = findNamedObject<wxToggleButton>(this, "ThreeWayMode");
    threeWayButton->SetBitmap(wxutil::GetLocalBitmap("three_way_merge.png"));
    threeWayButton->Bind(wxEVT_TOGGLEBUTTON, &MergeControlDialog::onMergeModeChanged, this);

    auto actionDescriptionPanel = findNamedObject<wxPanel>(this, "ActionDescriptionPanel");

    // Create the action list view
    _descriptionStore = new wxutil::TreeModel(COLUMNS(), true);
    auto listView = wxutil::TreeView::CreateWithModel(actionDescriptionPanel, _descriptionStore.get(), wxDV_NO_HEADER | wxDV_SINGLE);
    listView->SetMinClientSize(wxSize(-1, 70));
    listView->AppendTextColumn("-", COLUMNS().description.getColumnIndex(), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

    actionDescriptionPanel->GetSizer()->Add(listView, 1, wxEXPAND | wxBOTTOM, 6);

    updateControls();
    Bind(wxEVT_IDLE, &MergeControlDialog::onIdle, this);

    Layout();
    Fit();

    SetMinSize(wxSize(400, 290));
    InitialiseWindowPosition(400, 290, RKEY_WINDOW_STATE);
}

std::shared_ptr<MergeControlDialog>& MergeControlDialog::InstancePtr()
{
    static std::shared_ptr<MergeControlDialog> _instancePtr;
    return _instancePtr;
}

MergeControlDialog& MergeControlDialog::Instance()
{
    auto& instancePtr = InstancePtr();

    if (!instancePtr)
    {
        // Not yet instantiated, do it now
        instancePtr.reset(new MergeControlDialog);

        // Pre-destruction cleanup
        GlobalMainFrame().signal_MainFrameShuttingDown().connect(
            sigc::mem_fun(*instancePtr, &MergeControlDialog::onMainFrameShuttingDown)
        );
    }

    return *instancePtr;
}

void MergeControlDialog::onMainFrameShuttingDown()
{
    rMessage() << "MergeControlDialog shutting down." << std::endl;

    if (IsShownOnScreen())
    {
        Hide();
    }

    // Destroy the window 
    SendDestroyEvent();
    InstancePtr().reset();
}

bool MergeControlDialog::Show(bool show)
{
    // Check if there's a merge operation in progress
    // If yes, we ask the user before closing the window
    if (!show && IsShown() && GlobalMapModule().getEditMode() == IMap::EditMode::Merge)
    {
        if (wxutil::Messagebox::Show(_("Abort the Merge Operation?"),
            _("The current merge operation hasn't been finished yet.\nDo you want to abort the merge?"),
            IDialog::MessageType::MESSAGE_ASK, this) == IDialog::RESULT_NO)
        {
            return false; // block this call
        }
            
        // User wants to cancel, abort the merge and continue calling base
        GlobalMapModule().abortMergeOperation();
    }

    return TransientWindow::Show(show);
}

void MergeControlDialog::ShowDialog(const cmd::ArgumentList& args)
{
    Instance().Show();
}

void MergeControlDialog::convertTextCtrlToPathEntry(const std::string& ctrlName)
{
    auto oldCtrl = findNamedObject<wxTextCtrl>(this, ctrlName);
    replaceControl(oldCtrl, new wxutil::PathEntry(oldCtrl->GetParent(), "map", true));
}

void MergeControlDialog::onMergeSourceChanged(wxCommandEvent& ev)
{
    update();
}

bool MergeControlDialog::isInThreeWayMergeMode()
{
    return findNamedObject<wxToggleButton>(this, "ThreeWayMode")->GetValue();
}

void MergeControlDialog::setThreeWayMergeMode(bool enabled)
{
    findNamedObject<wxToggleButton>(this, "TwoWayMode")->SetValue(!enabled);
    findNamedObject<wxToggleButton>(this, "ThreeWayMode")->SetValue(enabled);

    findNamedObject<wxPanel>(this, "BaseMapPanel")->Show(enabled);
    findNamedObject<wxPanel>(this, "BaseMapPanel")->SetSize(wxSize(-1, enabled ? -1 : 0));
 
    update();

    InvalidateBestSize();
    Layout();
    Fit();
}

void MergeControlDialog::onMergeModeChanged(wxCommandEvent& ev)
{
    auto twoWayButton = findNamedObject<wxToggleButton>(this, "TwoWayMode");
    auto toggleButton = wxDynamicCast(ev.GetEventObject(), wxToggleButton);

    if (!toggleButton) return;

    if (toggleButton == twoWayButton)
    {
        setThreeWayMergeMode(!toggleButton->GetValue());
    }
    else // three-way button has been toggled
    {
        setThreeWayMergeMode(toggleButton->GetValue());
    }
}

void MergeControlDialog::onLoadAndCompare(wxCommandEvent& ev)
{
    auto sourceMapPath = findNamedObject<wxutil::PathEntry>(this, "MergeMapFilename")->getValue();
    auto baseMapPath = findNamedObject<wxutil::PathEntry>(this, "BaseMapFilename")->getValue();

    if (sourceMapPath.empty())
    {
        return;
    }

    bool isThreeWay = isInThreeWayMergeMode();

    if (isThreeWay && !baseMapPath.empty())
    { 
        GlobalMapModule().startMergeOperation(sourceMapPath, baseMapPath);
    }
    else if (!isThreeWay)
    {
        GlobalMapModule().startMergeOperation(sourceMapPath);
    }

    update();
}

void MergeControlDialog::onAbortMerge(wxCommandEvent& ev)
{
    GlobalMapModule().abortMergeOperation();
    update();
}

void MergeControlDialog::onResolveAccept(wxCommandEvent& ev)
{
    auto conflictNode = scene::merge::getSingleSelectedConflictNode();

    if (conflictNode)
    {
        conflictNode->foreachMergeAction([&](const scene::merge::IMergeAction::Ptr& action)
        {
            auto conflictAction = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(action);
            assert(conflictAction);
            
            conflictAction->setResolution(scene::merge::ResolutionType::ApplySourceChange);
        });
    }

    update();
}

void MergeControlDialog::onResolveReject(wxCommandEvent& ev)
{
    scene::merge::rejectSelectedNodesByDeletion();
    update();
}

void MergeControlDialog::onResolveKeepBoth(wxCommandEvent& ev)
{
    UndoableCommand undo("resolveMergeConflictByKeepingBothEntities");

    scene::merge::resolveConflictByKeepingBothEntities();
    update();
}

void MergeControlDialog::onJumpToNextConflict(wxCommandEvent& ev)
{
    scene::merge::focusNextConflictNode();
}

void MergeControlDialog::onFinishMerge(wxCommandEvent& ev)
{
    auto operation = GlobalMapModule().getActiveMergeOperation();

    if (operation)
    {
        auto mergeSelectionGroups = findNamedObject<wxCheckBox>(this, "KeepSelectionGroupsIntact")->GetValue();
        auto mergeLayers = findNamedObject<wxCheckBox>(this, "MergeLayers")->GetValue();
        operation->setMergeSelectionGroups(mergeSelectionGroups);
        operation->setMergeLayers(mergeLayers);

        GlobalMapModule().finishMergeOperation();

        if (GlobalMapModule().getEditMode() != IMap::EditMode::Merge)
        {
            // We're done here
            Hide();
            return;
        }
    }

    update();
}

inline void addActionDescription(const wxutil::TreeModel::Ptr& listStore, const scene::INodePtr& candidate)
{
    auto node = std::dynamic_pointer_cast<scene::IMergeActionNode>(candidate);

    if (!node) return;

    node->foreachMergeAction([&](const scene::merge::IMergeAction::Ptr& action)
    {
        if (!action->isActive()) return;

        std::string text = "-";

        auto conflictAction = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(action);

        if (conflictAction && conflictAction->getResolution() == scene::merge::ResolutionType::Unresolved)
        {
            switch (conflictAction->getConflictType())
            {
            case scene::merge::ConflictType::ModificationOfRemovedEntity:
                text = fmt::format(_("The imported map tries to modify the key values of the entity {0} that has already been deleted here."), 
                    scene::merge::getAffectedEntityName(conflictAction));
                break;
            case scene::merge::ConflictType::RemovalOfModifiedEntity:
                text = fmt::format(_("The imported map tries to remove the entity {0} that has been modified in this map."), 
                    scene::merge::getAffectedEntityName(conflictAction));
                break;
            case scene::merge::ConflictType::RemovalOfModifiedKeyValue:
                text = fmt::format(_("The imported map tries to remove the key {0} on entity {1} but this key has been modified in this map."),
                    scene::merge::getAffectedKeyName(conflictAction), scene::merge::getAffectedEntityName(conflictAction));
                break;
            case scene::merge::ConflictType::ModificationOfRemovedKeyValue:
                text = fmt::format(_("The imported map tries to modify the key \"{0}\" on entity {1} but this key has already been removed in this map."),
                    scene::merge::getAffectedKeyName(conflictAction), scene::merge::getAffectedEntityName(conflictAction));
                break;
            case scene::merge::ConflictType::SettingKeyToDifferentValue:
                text = fmt::format(_("The imported map tries to set the key \"{0}\" on entity {1} to the value \"{2}\" but this key has already been set to a different value in this map."),
                    scene::merge::getAffectedKeyName(conflictAction), scene::merge::getAffectedEntityName(conflictAction), scene::merge::getAffectedKeyValue(conflictAction));
                break;
            }
        }
        else
        {
            switch (action->getType())
            {
            case scene::merge::ActionType::AddEntity:
                text = fmt::format(_("The entity {0} will be added to this map."), scene::merge::getAffectedEntityName(action));
                break;
            case scene::merge::ActionType::RemoveEntity:
                text = fmt::format(_("The entity {0} will be removed from this map."), scene::merge::getAffectedEntityName(action));
                break;
            case scene::merge::ActionType::RemoveKeyValue:
                text = fmt::format(_("The key \"{0}\" will be removed from entity {1}."), scene::merge::getAffectedKeyName(action), scene::merge::getAffectedEntityName(action));
                break;
            case scene::merge::ActionType::AddKeyValue:
            case scene::merge::ActionType::ChangeKeyValue:
                text = fmt::format(_("The key \"{0}\" on entity {1} will be set to \"{2}\"."), scene::merge::getAffectedKeyName(action), 
                    scene::merge::getAffectedEntityName(action), scene::merge::getAffectedKeyValue(action));
                break;
            case scene::merge::ActionType::AddChildNode:
                text = fmt::format(_("A new primitive will be added to entity {0}."), scene::merge::getAffectedEntityName(action));
                break;
            case scene::merge::ActionType::RemoveChildNode:
                text = fmt::format(_("A primitive will be removed from {0}."), scene::merge::getAffectedEntityName(action));
                break;
            }
        }

        auto row = listStore->AddItem();
        row[COLUMNS().description] = text;

        row.SendItemAdded();
    });
}

void MergeControlDialog::updateControls()
{
    auto* targetMapFilename = findNamedObject<wxTextCtrl>(this, "TargetMapFilename");
    targetMapFilename->SetValue(os::getFilename(GlobalMapModule().getMapName()));
    targetMapFilename->Disable();

    auto selectedMergeNodes = scene::merge::getSelectedMergeNodes();
    bool mergeInProgress = GlobalMapModule().getEditMode() == IMap::EditMode::Merge;

    auto baseMapPathEntry = findNamedObject<wxutil::PathEntry>(this, "BaseMapFilename");
    auto sourceMapPathEntry = findNamedObject<wxutil::PathEntry>(this, "MergeMapFilename");

    auto baseMapPath = baseMapPathEntry->getValue();
    auto sourceMapPath = sourceMapPathEntry->getValue();

    // Fill in the map names in case the merge operation has already been started by the time the dialog is shown
    if (mergeInProgress)
    {
        auto mergeOperation = GlobalMapModule().getActiveMergeOperation();

        if (baseMapPath.empty() && mergeOperation && mergeOperation->getBasePath() != baseMapPath)
        {
            baseMapPath = mergeOperation->getBasePath();
            baseMapPathEntry->setValue(baseMapPath);
        }

        if (sourceMapPath.empty() && mergeOperation && mergeOperation->getSourcePath() != sourceMapPath)
        {
            sourceMapPath = mergeOperation->getSourcePath();
            sourceMapPathEntry->setValue(sourceMapPath);
        }
    }

    findNamedObject<wxWindow>(this, "TwoWayMode")->Enable(!mergeInProgress);
    findNamedObject<wxWindow>(this, "ThreeWayMode")->Enable(!mergeInProgress);

    findNamedObject<wxButton>(this, "AbortMergeButton")->Enable(mergeInProgress);
    findNamedObject<wxButton>(this, "FinishMergeButton")->Enable(mergeInProgress);
    findNamedObject<wxButton>(this, "LoadAndCompareButton")->Enable(!mergeInProgress && !sourceMapPath.empty() &&
        (!isInThreeWayMergeMode() || !baseMapPath.empty()));
    findNamedObject<wxWindow>(this, "SummaryPanel")->Enable(mergeInProgress);
    baseMapPathEntry->Enable(!mergeInProgress);
    sourceMapPathEntry->Enable(!mergeInProgress);

    auto actionDescriptionPanel = findNamedObject<wxPanel>(this, "ActionDescriptionPanel");

    auto conflictNode = scene::merge::getSingleSelectedConflictNode(selectedMergeNodes);

    findNamedObject<wxStaticText>(this, "NoMergeNodeSelected")->Show(selectedMergeNodes.size() != 1);
    findNamedObject<wxButton>(this, "ResolveAcceptButton")->Show(mergeInProgress && conflictNode != nullptr);
    findNamedObject<wxButton>(this, "ResolveKeepBothButton")->Show(mergeInProgress && 
        conflictNode != nullptr && scene::merge::canBeResolvedByKeepingBothEntities(conflictNode));
    findNamedObject<wxButton>(this, "ResolveRejectButton")->Show(mergeInProgress && !selectedMergeNodes.empty());
    findNamedObject<wxButton>(this, "JumpToNextConflictButton")->Enable(mergeInProgress && _numUnresolvedConflicts > 0);

    actionDescriptionPanel->Show(selectedMergeNodes.size() == 1);
    _descriptionStore->Clear();

    if (selectedMergeNodes.size() == 1)
    {
        addActionDescription(_descriptionStore, selectedMergeNodes.front());
    }

    InvalidateBestSize();
    Layout();
    Fit();
}

void MergeControlDialog::_preHide()
{
    TransientWindow::_preHide();

    // A hidden window doesn't need to listen for events
    _undoHandler.disconnect();
    _redoHandler.disconnect();
    _mapEventHandler.disconnect();

    GlobalSelectionSystem().removeObserver(this);
}

void MergeControlDialog::_preShow()
{
    TransientWindow::_preShow();

    _undoHandler.disconnect();
    _redoHandler.disconnect();

    // Register self to the SelSystem to get notified upon selection changes.
    GlobalSelectionSystem().addObserver(this);
    
    _mapEventHandler = GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(this, &MergeControlDialog::onMapEvent)
    );
    
    _undoHandler = GlobalMapModule().signal_postUndo().connect(
        sigc::mem_fun(this, &MergeControlDialog::queueUpdate));
    _redoHandler = GlobalMapModule().signal_postRedo().connect(
        sigc::mem_fun(this, &MergeControlDialog::queueUpdate));

    // Check for selection changes before showing the dialog again
    update();
}

void MergeControlDialog::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
    if (node->getNodeType() != scene::INode::Type::MergeAction)
    {
        return; // we only care about merge actions here
    }

    queueUpdate();
}

void MergeControlDialog::update()
{
    // Update the stats first to be able to enable/disable controls based on the numbers
    updateSummary();
    updateControls();
}

void MergeControlDialog::queueUpdate()
{
    _updateNeeded = true;
}

void MergeControlDialog::onIdle(wxIdleEvent& ev)
{
    if (!_updateNeeded) return;

    _updateNeeded = false;
    update();
}

void MergeControlDialog::onMapEvent(IMap::MapEvent ev)
{
    if (ev == IMap::MapLoaded || ev == IMap::MapUnloaded)
    {
        updateControls();
    }
}

void MergeControlDialog::OnMapEditModeChanged(IMap::EditMode mode)
{
    // When switching to merge mode, make sure the dialog is shown

    if (mode == IMap::EditMode::Merge && (!InstancePtr() || !Instance().IsShown()))
    {
        Instance().Show();
        return;
    }
    else if (InstancePtr() && Instance().IsShown())
    { 
        Instance().update();
    }
}

void MergeControlDialog::updateSummary()
{
    auto operation = GlobalMapModule().getActiveMergeOperation();

    std::size_t entitiesAdded = 0;
    std::size_t entitiesModified = 0;
    std::size_t entitiesRemoved = 0;
    std::size_t primitivesAdded = 0;
    std::size_t primitivesRemoved = 0;
    _numUnresolvedConflicts = 0;

    if (operation)
    {
        std::set<scene::INodePtr> modifiedEntities;

        operation->foreachAction([&](const scene::merge::IMergeAction::Ptr& action)
        {
            if (!action->isActive()) return;

            switch (action->getType())
            {
            case scene::merge::ActionType::AddChildNode:
                primitivesAdded++;
                break;
            case scene::merge::ActionType::RemoveChildNode:
                primitivesRemoved++;
                break;
            case scene::merge::ActionType::AddEntity:
                entitiesAdded++;
                break;
            case scene::merge::ActionType::RemoveEntity:
                entitiesRemoved++;
                break;
            case scene::merge::ActionType::AddKeyValue:
            case scene::merge::ActionType::ChangeKeyValue:
            case scene::merge::ActionType::RemoveKeyValue:
                modifiedEntities.insert(action->getAffectedNode());
                break;
            case scene::merge::ActionType::ConflictResolution:
            {
                auto conflict = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(action);

                if (conflict && conflict->getResolution() == scene::merge::ResolutionType::Unresolved)
                {
                    _numUnresolvedConflicts++;
                }
                break;
            }
            }
        });

        entitiesModified = modifiedEntities.size();
    }

    findNamedObject<wxStaticText>(this, "EntitiesAdded")->SetLabel(string::to_string(entitiesAdded));
    findNamedObject<wxStaticText>(this, "EntitiesRemoved")->SetLabel(string::to_string(entitiesRemoved));
    findNamedObject<wxStaticText>(this, "EntitiesModified")->SetLabel(string::to_string(entitiesModified));
    findNamedObject<wxStaticText>(this, "PrimitivesAdded")->SetLabel(string::to_string(primitivesAdded));
    findNamedObject<wxStaticText>(this, "PrimitivesRemoved")->SetLabel(string::to_string(primitivesRemoved));
    findNamedObject<wxStaticText>(this, "UnresolvedConflicts")->SetLabel(string::to_string(_numUnresolvedConflicts));

    if (_numUnresolvedConflicts > 0)
    {
        makeLabelBold(this, "UnresolvedConflicts");
        makeLabelBold(this, "UnresolvedConflictsLabel");

        findNamedObject<wxStaticText>(this, "UnresolvedConflicts")->SetForegroundColour(wxColour(200, 0, 0));
        findNamedObject<wxStaticText>(this, "UnresolvedConflictsLabel")->SetForegroundColour(wxColour(200, 0, 0));
    }
    else
    {
        auto label = findNamedObject<wxStaticText>(this, "UnresolvedConflicts");
        label->SetForegroundColour(wxNullColour);

        label = findNamedObject<wxStaticText>(this, "UnresolvedConflictsLabel");
        auto font = label->GetFont();
        font.SetWeight(wxFONTWEIGHT_NORMAL);
        label->SetFont(font);
        label->SetForegroundColour(wxNullColour);
    }
}

}
