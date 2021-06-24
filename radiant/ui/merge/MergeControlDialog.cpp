#include "MergeControlDialog.h"

#include "i18n.h"
#include "itextstream.h"
#include "imainframe.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "iundo.h"
#include "imap.h"
#include "wxutil/PathEntry.h"
#include "wxutil/Bitmap.h"
#include "wxutil/dialog/MessageBox.h"
#include "scenelib.h"
#include "string/convert.h"
#include "os/path.h"
#include "fmt/format.h"
#include "scene/merge/MergeAction.h"

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

    if (!baseMapPath.empty())
    { 
        GlobalMapModule().startMergeOperation(sourceMapPath, baseMapPath);
    }
    else
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
    auto conflictNode = getSingleSelectedConflictNode();

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

void MergeControlDialog::rejectSelectedNodesByDeletion()
{
    UndoableCommand undo("deleteSelectedMergeNodes");

    auto mergeNodes = getSelectedMergeNodes();

    for (const auto& mergeNode : mergeNodes)
    {
        scene::removeNodeFromParent(mergeNode);
    }

    update();
}

void MergeControlDialog::onResolveReject(wxCommandEvent& ev)
{
    UndoableCommand undo("deleteSelectedConflictNode");
    rejectSelectedNodesByDeletion();
}

void MergeControlDialog::onJumpToNextConflict(wxCommandEvent& ev)
{
    std::vector<std::shared_ptr<scene::IMergeActionNode>> mergeNodes;

    // Remove the selected nodes
    GlobalMapModule().getRoot()->foreachNode([&](const scene::INodePtr& node)
    {
        if (node->getNodeType() == scene::INode::Type::MergeAction)
        {
            auto mergeNode = std::dynamic_pointer_cast<scene::IMergeActionNode>(node);

            if (mergeNode && mergeNode->getActionType() == scene::merge::ActionType::ConflictResolution)
            {
                mergeNodes.push_back(mergeNode);
            }
        }

        return true;
    });

    if (mergeNodes.empty())
    {
        return;
    }

    scene::INodePtr current;
    
    if (GlobalSelectionSystem().countSelected() == 1 &&
        GlobalSelectionSystem().ultimateSelected()->getNodeType() == scene::INode::Type::MergeAction)
    {
        current = GlobalSelectionSystem().ultimateSelected();
    }

    auto nextNode = mergeNodes.front();
    auto currentNode = std::find(mergeNodes.begin(), mergeNodes.end(), current);
    
    if (currentNode != mergeNodes.end() && ++currentNode != mergeNodes.end())
    {
        nextNode = *currentNode;
    }

    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(nextNode, true);

    auto originAndAngles = scene::getOriginAndAnglesToLookAtNode(*nextNode->getAffectedNode());
    GlobalCommandSystem().executeCommand("FocusViews", cmd::ArgumentList{ originAndAngles.first, originAndAngles.second });
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

std::vector<scene::INodePtr> MergeControlDialog::getSelectedMergeNodes()
{
    std::vector<scene::INodePtr> mergeNodes;

    // Remove the selected nodes
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        if (node->getNodeType() == scene::INode::Type::MergeAction)
        {
            mergeNodes.push_back(node);
        }
    });

    return mergeNodes;
}

std::shared_ptr<scene::IMergeActionNode> MergeControlDialog::getSingleSelectedConflictNode()
{
    auto selectedNodes = getSelectedMergeNodes();

    if (selectedNodes.size() != 1)
    {
        return std::shared_ptr<scene::IMergeActionNode>();
    }

    auto mergeNode = std::dynamic_pointer_cast<scene::IMergeActionNode>(selectedNodes.front());

    if (mergeNode && mergeNode->getActionType() == scene::merge::ActionType::ConflictResolution)
    {
        return mergeNode;
    }

    return std::shared_ptr<scene::IMergeActionNode>();
}

std::size_t MergeControlDialog::getNumSelectedMergeNodes()
{
    return getSelectedMergeNodes().size();
}

inline std::string getEntityName(const scene::merge::IMergeAction::Ptr& action)
{
    auto entity = Node_getEntity(action->getAffectedNode());

    if (entity == nullptr && action->getAffectedNode())
    {
        entity = Node_getEntity(action->getAffectedNode()->getParent());
    }

    return entity ? (entity->isWorldspawn() ? "worldspawn" : entity->getKeyValue("name")) : "?";
}

inline std::string getKeyName(const scene::merge::IMergeAction::Ptr& action)
{
    auto conflictAction = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(action);

    auto keyValueAction = std::dynamic_pointer_cast<scene::merge::IEntityKeyValueMergeAction>(
        conflictAction ? conflictAction->getSourceAction() : action);

    return keyValueAction ? keyValueAction->getKey() : std::string();
}

inline std::string getKeyValue(const scene::merge::IMergeAction::Ptr& action)
{
    auto conflictAction = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(action);

    auto keyValueAction = std::dynamic_pointer_cast<scene::merge::IEntityKeyValueMergeAction>(
        conflictAction ? conflictAction->getSourceAction() : action);

    return keyValueAction ? keyValueAction->getValue() : std::string();
}

inline void addActionDescription(wxPanel* panel, const scene::INodePtr& candidate)
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
                text = fmt::format(_("The imported map tries to modify the key values of the entity {0} that has already been deleted here."), getEntityName(conflictAction));
                break;
            case scene::merge::ConflictType::RemovalOfModifiedEntity:
                text = fmt::format(_("The imported map tries to remove the entity {0} that has been modified in this map."), getEntityName(conflictAction));
                break;
            case scene::merge::ConflictType::RemovalOfModifiedKeyValue:
                text = fmt::format(_("The imported map tries to remove the key {0} on entity {1} but this key has been modified in this map."),
                    getKeyName(conflictAction), getEntityName(conflictAction));
                break;
            case scene::merge::ConflictType::ModificationOfRemovedKeyValue:
                text = fmt::format(_("The imported map tries to modify the key \"{0}\" on entity {1} but this key has already been removed in this map."),
                    getKeyName(conflictAction), getEntityName(conflictAction));
                break;
            case scene::merge::ConflictType::SettingKeyToDifferentValue:
                text = fmt::format(_("The imported map tries to set the key \"{0}\" on entity {1} to the value \"{2}\" but this key has already been set to a different value in this map."),
                    getKeyName(conflictAction), getEntityName(conflictAction), getKeyValue(conflictAction));
                break;
            }
        }
        else
        {
            switch (action->getType())
            {
            case scene::merge::ActionType::AddEntity:
                text = fmt::format(_("The entity {0} will be added to this map."), getEntityName(action));
                break;
            case scene::merge::ActionType::RemoveEntity:
                text = fmt::format(_("The entity {0} will be removed from this map."), getEntityName(action));
                break;
            case scene::merge::ActionType::RemoveKeyValue:
                text = fmt::format(_("The key \"{0}\" will be removed from entity {1}."), getKeyName(action), getEntityName(action));
                break;
            case scene::merge::ActionType::AddKeyValue:
            case scene::merge::ActionType::ChangeKeyValue:
                text = fmt::format(_("The key \"{0}\" on entity {1} will be set to \"{2}\"."), getKeyName(action), getEntityName(action), getKeyValue(action));
                break;
            case scene::merge::ActionType::AddChildNode:
                text = fmt::format(_("A new primitive will be added to entity {0}."), getEntityName(action));
                break;
            case scene::merge::ActionType::RemoveChildNode:
                text = fmt::format(_("A primitive will be removed from {0}."), getEntityName(action));
                break;
            }
        }

        auto* staticText = new wxStaticText(panel, wxID_ANY, text);
        staticText->Wrap(panel->GetSize().x);
        staticText->Layout();
        panel->GetSizer()->Add(staticText, 0, wxBOTTOM, 6);
    });
}

void MergeControlDialog::updateControls()
{
    auto* targetMapFilename = findNamedObject<wxTextCtrl>(this, "TargetMapFilename");
    targetMapFilename->SetValue(os::getFilename(GlobalMapModule().getMapName()));
    targetMapFilename->Disable();

    auto selectedMergeNodes = getSelectedMergeNodes();
    bool mergeInProgress = GlobalMapModule().getEditMode() == IMap::EditMode::Merge;
    auto baseMapPath = findNamedObject<wxutil::PathEntry>(this, "BaseMapFilename")->getValue();
    auto sourceMapPath = findNamedObject<wxutil::PathEntry>(this, "MergeMapFilename")->getValue();

    findNamedObject<wxWindow>(this, "TwoWayMode")->Enable(!mergeInProgress);
    findNamedObject<wxWindow>(this, "ThreeWayMode")->Enable(!mergeInProgress);

    findNamedObject<wxButton>(this, "AbortMergeButton")->Enable(mergeInProgress);
    findNamedObject<wxButton>(this, "FinishMergeButton")->Enable(mergeInProgress);
    findNamedObject<wxButton>(this, "LoadAndCompareButton")->Enable(!mergeInProgress && !sourceMapPath.empty() &&
        (!isInThreeWayMergeMode() || !baseMapPath.empty()));
    findNamedObject<wxWindow>(this, "SummaryPanel")->Enable(mergeInProgress);
    findNamedObject<wxWindow>(this, "BaseMapFilename")->Enable(!mergeInProgress);
    findNamedObject<wxWindow>(this, "MergeMapFilename")->Enable(!mergeInProgress);

    auto actionDescriptionPanel = findNamedObject<wxPanel>(this, "ActionDescriptionPanel");

    auto conflictNode = getSingleSelectedConflictNode();

    findNamedObject<wxStaticText>(this, "NoMergeNodeSelected")->Show(selectedMergeNodes.size() != 1);
    findNamedObject<wxButton>(this, "ResolveAcceptButton")->Show(mergeInProgress && conflictNode != nullptr);
    findNamedObject<wxButton>(this, "ResolveRejectButton")->Show(mergeInProgress && !selectedMergeNodes.empty());
    findNamedObject<wxButton>(this, "JumpToNextConflictButton")->Enable(mergeInProgress && _numUnresolvedConflicts > 0);

    actionDescriptionPanel->Show(selectedMergeNodes.size() == 1);
    actionDescriptionPanel->DestroyChildren();

    if (selectedMergeNodes.size() == 1)
    {
        addActionDescription(actionDescriptionPanel, selectedMergeNodes.front());
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
    _mapEditModeHandler.disconnect();

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
    _mapEditModeHandler = GlobalMapModule().signal_editModeChanged().connect(
        sigc::mem_fun(this, &MergeControlDialog::onMapEditModeChanged)
    );

    _undoHandler = GlobalUndoSystem().signal_postUndo().connect(
        sigc::mem_fun(this, &MergeControlDialog::queueUpdate));
    _redoHandler = GlobalUndoSystem().signal_postRedo().connect(
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

void MergeControlDialog::onMapEditModeChanged(IMap::EditMode newMode)
{
    update();
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
