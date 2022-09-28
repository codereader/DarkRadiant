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
	wxButton* _renameButton;
	wxButton* _deleteButton;

	bool _refreshTreeOnIdle;
	bool _updateTreeOnIdle;
	bool _rescanSelectionOnIdle;

	sigc::connection _selectionChangedSignal;
	sigc::connection _layersChangedSignal;
	sigc::connection _layerVisibilityChangedSignal;
	sigc::connection _nodeLayerMembershipChangedSignal;
	sigc::connection _mapEventSignal;

public:
    LayerControlDialog();

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

    // Calls refresh() on the next idle event
    void queueRefresh();

    // Calls update() on the next idle event
    void queueUpdate();

    // Rebuilds the whole data view
    void refresh();

    // Updates the state of all tree items, doesn't clear the tree
    void update();

	void populateWindow();
	void clearControls();

	// Update the usage colours on the controls
	void updateLayerUsage();
	void updateButtonSensitivity(std::size_t numVisible, std::size_t numHidden);
	void updateItemActionSensitivity();

	// Creates the option buttons
	void createButtons();

	void setVisibilityOfAllLayers(bool visible);
	void onIdle();

	void onMapEvent(IMap::MapEvent ev);

	void connectToMapRoot();
	void disconnectFromMapRoot();

    void onItemActivated(wxDataViewEvent& ev);
    void onItemToggled(wxDataViewEvent& ev);
    void onItemSelected(wxDataViewEvent& ev);
    void onBeginDrag(wxDataViewEvent& ev);
    void onDropPossible(wxDataViewEvent& ev);
    void onDrop(wxDataViewEvent& ev);
    void onRenameLayer(wxCommandEvent& ev);
    void onDeleteLayer(wxCommandEvent& ev);
    int getSelectedLayerId();
};

} // namespace ui
