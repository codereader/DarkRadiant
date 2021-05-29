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

#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>

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
    targetMapFilename->SetValue(GlobalMapModule().getMapName());
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

    updateControlSensitivity();
    Bind(wxEVT_IDLE, &MergeControlDialog::onIdle, this);

    SetMinSize(wxSize(300, 400));
    InitialiseWindowPosition(300, 400, RKEY_WINDOW_STATE);
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

void MergeControlDialog::Toggle(const cmd::ArgumentList& args)
{
    Instance().ToggleVisibility();
}

void MergeControlDialog::convertTextCtrlToPathEntry(const std::string& ctrlName)
{
    auto oldCtrl = findNamedObject<wxTextCtrl>(this, ctrlName);
    replaceControl(oldCtrl, new wxutil::PathEntry(oldCtrl->GetParent(), false));
}

void MergeControlDialog::onMergeSourceChanged(wxCommandEvent& ev)
{
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
        GlobalCommandSystem().executeCommand("StartMergeOperation", sourceMapPath, baseMapPath);
    }
    else
    {
        GlobalCommandSystem().executeCommand("StartMergeOperation", sourceMapPath);
    }

    updateControlSensitivity();
}

void MergeControlDialog::onAbortMerge(wxCommandEvent& ev)
{
    GlobalMapModule().abortMergeOperation();
    updateControlSensitivity();
}

void MergeControlDialog::onRejectSelection(wxCommandEvent& ev)
{
    UndoableCommand undo("deleteSelectedMergeNodes");

    auto mergeNodes = getSelectedMergeNodes();

    for (const auto& mergeNode : mergeNodes)
    {
        scene::removeNodeFromParent(mergeNode);
    }

    updateControlSensitivity();
}

void MergeControlDialog::onFinishMerge(wxCommandEvent& ev)
{
    GlobalMapModule().finishMergeOperation();

    if (GlobalMapModule().getEditMode() != IMap::EditMode::Merge)
    {
        // We're done here
        Hide();
        return;
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

std::size_t MergeControlDialog::getNumSelectedMergeNodes()
{
    return getSelectedMergeNodes().size();
}

void MergeControlDialog::updateControlSensitivity()
{
    auto numSelectedMergeNodes = getNumSelectedMergeNodes();
    bool mergeInProgress = GlobalMapModule().getEditMode() == IMap::EditMode::Merge;
    auto sourceMapPath = findNamedObject<wxutil::PathEntry>(this, "MergeMapFilename")->getValue();

    findNamedObject<wxButton>(this, "AbortMergeButton")->Enable(mergeInProgress);
    findNamedObject<wxButton>(this, "FinishMergeButton")->Enable(mergeInProgress);
    findNamedObject<wxButton>(this, "LoadAndCompareButton")->Enable(!mergeInProgress && !sourceMapPath.empty());
    findNamedObject<wxWindow>(this, "BaseMapFilename")->Enable(!mergeInProgress);
    findNamedObject<wxWindow>(this, "MergeMapFilename")->Enable(!mergeInProgress);

    findNamedObject<wxButton>(this, "RejectSelectionButton")->Enable(mergeInProgress && numSelectedMergeNodes > 0);
}

void MergeControlDialog::_preHide()
{
    TransientWindow::_preHide();

    // A hidden window doesn't need to listen for events
    _undoHandler.disconnect();
    _redoHandler.disconnect();

    GlobalSelectionSystem().removeObserver(this);
}

void MergeControlDialog::_preShow()
{
    TransientWindow::_preShow();

    _undoHandler.disconnect();
    _redoHandler.disconnect();

    // Register self to the SelSystem to get notified upon selection changes.
    GlobalSelectionSystem().addObserver(this);

    _undoHandler = GlobalUndoSystem().signal_postUndo().connect(
        sigc::mem_fun(this, &MergeControlDialog::queueUpdate));
    _redoHandler = GlobalUndoSystem().signal_postRedo().connect(
        sigc::mem_fun(this, &MergeControlDialog::queueUpdate));

    // Check for selection changes before showing the dialog again
    updateControlSensitivity();
}

void MergeControlDialog::rescanSelection()
{
    
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
    rescanSelection();
    updateControlSensitivity();
}

}
