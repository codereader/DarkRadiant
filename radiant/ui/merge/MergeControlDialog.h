#pragma once

#include "iselection.h"
#include "imap.h"
#include "icommandsystem.h"
#include <sigc++/connection.h>
#include "wxutil/window/TransientWindow.h"
#include "wxutil/XmlResourceBasedWidget.h"

namespace ui
{


class MergeControlDialog final :
    public wxutil::TransientWindow,
    private wxutil::XmlResourceBasedWidget,
    public SelectionSystem::Observer,
    public sigc::trackable
{
private:
    sigc::connection _undoHandler;
    sigc::connection _redoHandler;
    sigc::connection _mapEventHandler;
    sigc::connection _mapEditModeHandler;

    bool _updateNeeded;
    std::size_t _numUnresolvedConflicts;

public:
    MergeControlDialog();

    static MergeControlDialog& Instance();

    bool Show(bool show = true) override;

    // The command target
    static void ShowDialog(const cmd::ArgumentList& args);

    /** greebo: SelectionSystem::Observer implementation. Gets called by
     * the SelectionSystem upon selection change to allow updating of the
     * patch property widgets.
     */
    void selectionChanged(const scene::INodePtr& node, bool isComponent) override;

    bool isInThreeWayMergeMode();
    void setThreeWayMergeMode(bool enabled);

protected:
    void _preShow() override;
    void _preHide() override;

private:
    void onMainFrameShuttingDown();
    static std::shared_ptr<MergeControlDialog>& InstancePtr();

    void convertTextCtrlToPathEntry(const std::string& ctrlName);
    void onMergeSourceChanged(wxCommandEvent& ev);
    void onMergeModeChanged(wxCommandEvent& ev);
    void onLoadAndCompare(wxCommandEvent& ev);
    void onFinishMerge(wxCommandEvent& ev);
    void onAbortMerge(wxCommandEvent& ev);
    void onResolveAccept(wxCommandEvent& ev);
    void onResolveReject(wxCommandEvent& ev);
    void onResolveKeepBoth(wxCommandEvent& ev);
    void onJumpToNextConflict(wxCommandEvent& ev);
    void updateControls();
    void queueUpdate();
    void onIdle(wxIdleEvent& ev);
    void onMapEvent(IMap::MapEvent ev);
    void onMapEditModeChanged(IMap::EditMode newMode);
    void updateSummary();

    void update();
};

}
