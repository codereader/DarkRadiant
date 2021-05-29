#include "MergeControlDialog.h"

#include "i18n.h"
#include "itextstream.h"
#include "imainframe.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "iundo.h"
#include "imap.h"
#include "wxutil/PathEntry.h"

#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Merge Maps");
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

    updateControlSensitivity();

    SetMinSize(wxSize(300, 300));

    Bind(wxEVT_IDLE, &MergeControlDialog::onIdle, this);
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

void MergeControlDialog::updateControlSensitivity()
{
    bool mergeInProgress = GlobalMapModule().getEditMode() == IMap::EditMode::Merge;
    auto sourceMapPath = findNamedObject<wxutil::PathEntry>(this, "MergeMapFilename")->getValue();

    findNamedObject<wxButton>(this, "AbortMergeButton")->Enable(mergeInProgress);
    findNamedObject<wxButton>(this, "LoadAndCompareButton")->Enable(!mergeInProgress && !sourceMapPath.empty());
    findNamedObject<wxWindow>(this, "BaseMapFilename")->Enable(!mergeInProgress);
    findNamedObject<wxWindow>(this, "MergeMapFilename")->Enable(!mergeInProgress);
}

// Pre-hide callback
void MergeControlDialog::_preHide()
{
    TransientWindow::_preHide();

    // A hidden PatchInspector doesn't need to listen for events
    _undoHandler.disconnect();
    _redoHandler.disconnect();

    GlobalSelectionSystem().removeObserver(this);
}

// Pre-show callback
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
    rescanSelection();
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

    rescanSelection();
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
}

}
