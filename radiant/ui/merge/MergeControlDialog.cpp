#include "MergeControlDialog.h"

#include "i18n.h"
#include "itextstream.h"
#include "imainframe.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "iundo.h"
#include "imap.h"
#include "wxutil/PathEntry.h"
#include "scenelib.h"
#include "string/convert.h"
#include "os/path.h"
#include "fmt/format.h"

#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

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
    _updateNeeded(false)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));
    GetSizer()->Add(loadNamedPanel(this, "MergeControlDialogMainPanel"), 1, wxEXPAND);

    auto* targetMapFilename = findNamedObject<wxTextCtrl>(this, "TargetMapFilename");
    targetMapFilename->SetValue(os::getFilename(GlobalMapModule().getMapName()));
    targetMapFilename->Disable();

    convertTextCtrlToPathEntry("MergeMapFilename");
    convertTextCtrlToPathEntry("BaseMapFilename");

    auto* mergeSourceEntry = findNamedObject<wxutil::PathEntry>(this, "MergeMapFilename");
    mergeSourceEntry->Bind(wxutil::EV_PATH_ENTRY_CHANGED, &MergeControlDialog::onMergeSourceChanged, this);

    auto* abortMergeButton = findNamedObject<wxButton>(this, "AbortMergeButton");
    abortMergeButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onAbortMerge, this);

    auto* loadAndCompareButton = findNamedObject<wxButton>(this, "LoadAndCompareButton");
    loadAndCompareButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onLoadAndCompare, this);

    auto* rejectButton = findNamedObject<wxButton>(this, "RejectSelectionButton");
    rejectButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onRejectSelection, this);

    auto* finishButton = findNamedObject<wxButton>(this, "FinishMergeButton");
    finishButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onFinishMerge, this);

    auto* resolveAcceptButton = findNamedObject<wxButton>(this, "ResolveAcceptButton");
    resolveAcceptButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onResolveAccept, this);

    auto* resolveRejectButton = findNamedObject<wxButton>(this, "ResolveRejectButton");
    resolveRejectButton->Bind(wxEVT_BUTTON, &MergeControlDialog::onResolveReject, this);

    findNamedObject<wxCheckBox>(this, "KeepSelectionGroupsIntact")->SetValue(true);
    findNamedObject<wxCheckBox>(this, "MergeLayers")->SetValue(true);

    updateControlSensitivity();
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
    updateSummary();
    updateControlSensitivity();
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

    updateSummary();
    updateControlSensitivity();
}

void MergeControlDialog::onAbortMerge(wxCommandEvent& ev)
{
    GlobalMapModule().abortMergeOperation();
    updateSummary();
    updateControlSensitivity();
}

void MergeControlDialog::onRejectSelection(wxCommandEvent& ev)
{
    rejectSelectedNodesByDeletion();
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

    updateSummary();
    updateControlSensitivity();
}

void MergeControlDialog::rejectSelectedNodesByDeletion()
{
    UndoableCommand undo("deleteSelectedMergeNodes");

    auto mergeNodes = getSelectedMergeNodes();

    for (const auto& mergeNode : mergeNodes)
    {
        scene::removeNodeFromParent(mergeNode);
    }

    updateSummary();
    updateControlSensitivity();
}

void MergeControlDialog::onResolveReject(wxCommandEvent& ev)
{
    UndoableCommand undo("deleteSelectedConflictNode");
    rejectSelectedNodesByDeletion();
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

    updateControlSensitivity();
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

inline std::string getEntityName(const scene::merge::IConflictResolutionAction::Ptr& action)
{
    auto entity = Node_getEntity(action->getConflictingEntity());
    return entity ? entity->getKeyValue("name") : "?";
}

inline std::string getKeyName(const scene::merge::IConflictResolutionAction::Ptr& action)
{
    auto keyConflictAction = std::dynamic_pointer_cast<scene::merge::IEntityKeyValueMergeAction>(action->getSourceAction());
    return keyConflictAction ? keyConflictAction->getKey() : std::string();
}

inline std::string getKeyValue(const scene::merge::IConflictResolutionAction::Ptr& action)
{
    auto keyConflictAction = std::dynamic_pointer_cast<scene::merge::IEntityKeyValueMergeAction>(action->getSourceAction());
    return keyConflictAction ? keyConflictAction->getValue() : std::string();
}

inline void addConflictDescription(wxPanel* panel, const std::shared_ptr<scene::IMergeActionNode>& node)
{
    if (!node) return;

    node->foreachMergeAction([&](const scene::merge::IMergeAction::Ptr& action)
    {
        if (action->getType() != scene::merge::ActionType::ConflictResolution || !action->isActive())
        {
            return;
        }

        auto conflictAction = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(action);

        auto* text = new wxStaticText(panel, wxID_ANY, "");
        
        switch (conflictAction->getConflictType())
        {
        case scene::merge::ConflictType::ModificationOfRemovedEntity:
            text->SetLabel(fmt::format(_("The imported map tries to modify the key values of the entity {0} that has already been deleted here."), getEntityName(conflictAction)));
            break;
        case scene::merge::ConflictType::RemovalOfModifiedEntity:
            text->SetLabel(fmt::format(_("The imported map tries to remove the entity {0} that has been modified in this map."), getEntityName(conflictAction)));
            break;
        case scene::merge::ConflictType::RemovalOfModifiedKeyValue:
            text->SetLabel(fmt::format(_("The imported map tries to remove the key {0} on entity {1} but this key has been modified in this map."),
                getKeyName(conflictAction), getEntityName(conflictAction)));
            break;
        case scene::merge::ConflictType::ModificationOfRemovedKeyValue:
            text->SetLabel(fmt::format(_("The imported map tries to modify the key \"{0}\" on entity {1} but this key has already been removed in this map."),
                getKeyName(conflictAction), getEntityName(conflictAction)));
            break;
        case scene::merge::ConflictType::SettingKeyToDifferentValue:
            text->SetLabel(fmt::format(_("The imported map tries to set the key \"{0}\" on entity {1} to the value \"{2}\" but this key has already been set to a different value in this map."),
                getKeyName(conflictAction), getEntityName(conflictAction), getKeyValue(conflictAction)));
            break;
        }

        text->Wrap(panel->GetSize().x);
        panel->GetSizer()->Add(text, 0, wxBOTTOM, 6);
    });
}

void MergeControlDialog::updateControlSensitivity()
{
    auto numSelectedMergeNodes = getNumSelectedMergeNodes();
    bool mergeInProgress = GlobalMapModule().getEditMode() == IMap::EditMode::Merge;
    auto sourceMapPath = findNamedObject<wxutil::PathEntry>(this, "MergeMapFilename")->getValue();

    findNamedObject<wxButton>(this, "AbortMergeButton")->Enable(mergeInProgress);
    findNamedObject<wxButton>(this, "FinishMergeButton")->Enable(mergeInProgress);
    findNamedObject<wxButton>(this, "LoadAndCompareButton")->Enable(!mergeInProgress && !sourceMapPath.empty());
    findNamedObject<wxWindow>(this, "SummaryPanel")->Enable(mergeInProgress);
    findNamedObject<wxWindow>(this, "BaseMapFilename")->Enable(!mergeInProgress);
    findNamedObject<wxWindow>(this, "MergeMapFilename")->Enable(!mergeInProgress);
    findNamedObject<wxButton>(this, "RejectSelectionButton")->Enable(mergeInProgress && numSelectedMergeNodes > 0);

    auto conflictDescriptionPanel = findNamedObject<wxPanel>(this, "ConflictDescriptionPanel");

    auto conflictNode = getSingleSelectedConflictNode();

    findNamedObject<wxStaticText>(this, "NoMergeConflictSelected")->Show(conflictNode == nullptr);
    findNamedObject<wxButton>(this, "ResolveAcceptButton")->Show(conflictNode != nullptr);
    findNamedObject<wxButton>(this, "ResolveRejectButton")->Show(conflictNode != nullptr);
    conflictDescriptionPanel->Show(conflictNode != nullptr);

    conflictDescriptionPanel->DestroyChildren();

    if (conflictNode)
    {
        addConflictDescription(conflictDescriptionPanel, conflictNode);
    }

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
    updateControlSensitivity();
    updateSummary(); 
}

void MergeControlDialog::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
    if (node->getNodeType() != scene::INode::Type::MergeAction)
    {
        return; // we only care about merge actions here
    }

    queueUpdate();
}

void MergeControlDialog::queueUpdate()
{
    _updateNeeded = true;
}

void MergeControlDialog::onIdle(wxIdleEvent& ev)
{
    if (!_updateNeeded) return;

    _updateNeeded = false;
    updateSummary();
    updateControlSensitivity();
}

void MergeControlDialog::onMapEvent(IMap::MapEvent ev)
{
    if (ev == IMap::MapLoaded || ev == IMap::MapUnloaded)
    {
        updateControlSensitivity();
    }
}

void MergeControlDialog::onMapEditModeChanged(IMap::EditMode newMode)
{
    updateSummary();
    updateControlSensitivity();
}

void MergeControlDialog::updateSummary()
{
    auto operation = GlobalMapModule().getActiveMergeOperation();

    std::size_t entitiesAdded = 0;
    std::size_t entitiesModified = 0;
    std::size_t entitiesRemoved = 0;
    std::size_t primitivesAdded = 0;
    std::size_t primitivesRemoved = 0;
    std::size_t unresolvedConflicts = 0;

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
                    unresolvedConflicts++;
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
    findNamedObject<wxStaticText>(this, "UnresolvedConflicts")->SetLabel(string::to_string(unresolvedConflicts));

    if (unresolvedConflicts > 0)
    {
        makeLabelBold(this, "UnresolvedConflicts");
        makeLabelBold(this, "UnresolvedConflictsLabel");

        findNamedObject<wxStaticText>(this, "UnresolvedConflicts")->SetForegroundColour(wxColour(200, 0, 0));
        findNamedObject<wxStaticText>(this, "UnresolvedConflictsLabel")->SetForegroundColour(wxColour(200, 0, 0));
    }
    else
    {
        auto label = findNamedObject<wxStaticText>(this, "UnresolvedConflicts");
        auto font = label->GetFont();
        font.SetWeight(wxFONTWEIGHT_NORMAL);
        label->SetFont(font);
        label->SetForegroundColour(wxNullColour);

        label = findNamedObject<wxStaticText>(this, "UnresolvedConflictsLabel");
        font = label->GetFont();
        font.SetWeight(wxFONTWEIGHT_NORMAL);
        label->SetFont(font);
        label->SetForegroundColour(wxNullColour);
    }

    Layout();
    Fit();
}

}
