#pragma once

#include "iselection.h"
#include "imap.h"
#include "icommandsystem.h"
#include <sigc++/connection.h>
#include "wxutil/window/TransientWindow.h"
#include "wxutil/XmlResourceBasedWidget.h"

namespace ui
{


class MergeControlDialog :
    public wxutil::TransientWindow,
    private wxutil::XmlResourceBasedWidget,
    public SelectionSystem::Observer,
    public sigc::trackable
{
private:
    sigc::connection _undoHandler;
    sigc::connection _redoHandler;
    sigc::connection _mapEventHandler;

    bool _updateNeeded;

public:
    MergeControlDialog();

    static MergeControlDialog& Instance();

    // The command target
    static void ShowDialog(const cmd::ArgumentList& args);

    /** greebo: SelectionSystem::Observer implementation. Gets called by
     * the SelectionSystem upon selection change to allow updating of the
     * patch property widgets.
     */
    void selectionChanged(const scene::INodePtr& node, bool isComponent) override;

protected:
    void _preShow() override;
    void _preHide() override;

private:
    void onMainFrameShuttingDown();
    static std::shared_ptr<MergeControlDialog>& InstancePtr();

    void convertTextCtrlToPathEntry(const std::string& ctrlName);
    void onMergeSourceChanged(wxCommandEvent& ev);
    void onLoadAndCompare(wxCommandEvent& ev);
    void onFinishMerge(wxCommandEvent& ev);
    void onAbortMerge(wxCommandEvent& ev);
    void onRejectSelection(wxCommandEvent& ev);
    void updateControlSensitivity();
    void queueUpdate();
    void onIdle(wxIdleEvent& ev);
    void onMapEvent(IMap::MapEvent ev);

    std::size_t getNumSelectedMergeNodes();
    std::vector<scene::INodePtr> getSelectedMergeNodes();
};

}
