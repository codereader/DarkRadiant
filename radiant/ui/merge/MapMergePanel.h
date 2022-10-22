#pragma once

#include "iselection.h"
#include "imap.h"
#include <sigc++/connection.h>
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/dataview/TreeModel.h"
#include "wxutil/event/SingleIdleCallback.h"

#include "wxutil/DockablePanel.h"

namespace ui
{

class MapMergePanel final :
    public wxutil::DockablePanel,
    private wxutil::XmlResourceBasedWidget,
    public selection::SelectionSystem::Observer,
    public sigc::trackable,
    public wxutil::SingleIdleCallback
{
private:
    sigc::connection _undoHandler;
    sigc::connection _redoHandler;
    sigc::connection _mapEventHandler;
    sigc::connection _mapEditModeChangedHandler;

    bool _updateNeeded;
    std::size_t _numUnresolvedConflicts;

    wxutil::TreeModel::Ptr _descriptionStore;

public:
    MapMergePanel(wxWindow* parent);
    ~MapMergePanel() override;

    /** greebo: SelectionSystem::Observer implementation. Gets called by
     * the SelectionSystem upon selection change to allow updating of the
     * patch property widgets.
     */
    void selectionChanged(const scene::INodePtr& node, bool isComponent) override;

    bool isInThreeWayMergeMode();
    void setThreeWayMergeMode(bool enabled);

    static void OnMapEditModeChanged(IMap::EditMode newMode);

protected:
    void onIdle() override;
    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectListeners();
    void disconnectListeners();

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
    void onMapEvent(IMap::MapEvent ev);
    void onClose(wxCloseEvent& ev);
    void updateSummary();

    void update();
};

}
