#pragma once

#include "imap.h"
#include "icommandsystem.h"
#include "wxutil/window/TransientWindow.h"

#include <wx/panel.h>
#include <sigc++/connection.h>

#include "wxutil/dataview/TreeModel.h"

namespace wxutil
{
    class TreeView;
}

class wxButton;
class wxFlexGridSizer;
class wxWindow;
class wxCommandEvent;

namespace ui
{

class LayerControlDialog :
	public wxutil::TransientWindow
{
private:
    struct TreeColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        TreeColumns() :
            id(add(wxutil::TreeModel::Column::Integer)),
            visible(add(wxutil::TreeModel::Column::Boolean)),
            name(add(wxutil::TreeModel::Column::String)),
            selectionIsPartOfLayer(add(wxutil::TreeModel::Column::Boolean))
        {}

        wxutil::TreeModel::Column id;
        wxutil::TreeModel::Column visible;
        wxutil::TreeModel::Column name;
        wxutil::TreeModel::Column selectionIsPartOfLayer;
    };

    wxutil::TreeView* _layersView;
    TreeColumns _columns;
    wxutil::TreeModel::Ptr _layerStore;

    std::map<int, wxDataViewItem> _layerItemMap;

	wxButton* _showAllLayers;
	wxButton* _hideAllLayers;

	bool _rescanSelectionOnIdle;
	sigc::connection _selectionChangedSignal;
	sigc::connection _layersChangedSignal;
	sigc::connection _layerVisibilityChangedSignal;
	sigc::connection _nodeLayerMembershipChangedSignal;
	sigc::connection _mapEventSignal;

public:
    LayerControlDialog();

	// Re-populates the window
	void refresh();

	// Updates the state of all LayerControls
	void update();

	// Command target (registered in the event manager)
	static void ToggleDialog(const cmd::ArgumentList& args);

	// Checks if dialog should be shown after startup
	static void onMainFrameConstructed();

	static LayerControlDialog& Instance();

private:
	static std::shared_ptr<LayerControlDialog>& InstancePtr();

	void onMainFrameShuttingDown();

	// TransientWindow events
	void _preShow() override;
	void _postHide() override;

	void populateWindow();
	void clearControls();

	// Update the usage colours on the controls
	void updateLayerUsage();

	// Creates the option buttons
	void createButtons();

	void onShowAllLayers(wxCommandEvent& ev);
	void onHideAllLayers(wxCommandEvent& ev);
	void onIdle();

	void onMapEvent(IMap::MapEvent ev);

	void connectToMapRoot();
	void disconnectFromMapRoot();
};

} // namespace ui
