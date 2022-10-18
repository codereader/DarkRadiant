#pragma once

#include <map>
#include "imap.h"

#include <wx/panel.h>
#include <sigc++/connection.h>

#include "wxutil/dataview/TreeModel.h"
#include "wxutil/event/SingleIdleCallback.h"

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

class LayerControlPanel :
	public wxPanel,
    public wxutil::SingleIdleCallback
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

    class TreePopulator;

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
	sigc::connection _layerHierarchyChangedSignal;
	sigc::connection _mapEventSignal;

public:
    LayerControlPanel(wxWindow* parent);
    ~LayerControlPanel() override;

protected:
    void onIdle() override;

private:
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

	void onMapEvent(IMap::MapEvent ev);

	void connectToMapRoot();
	void disconnectFromMapRoot();

    void onItemActivated(wxDataViewEvent& ev);
    void onItemValueChanged(wxDataViewEvent& ev);
    void onItemSelected(wxDataViewEvent& ev);
    void onBeginDrag(wxDataViewEvent& ev);
    void onDropPossible(wxDataViewEvent& ev);
    void onDrop(wxDataViewEvent& ev);
    void onRenameLayer(wxCommandEvent& ev);
    void onDeleteLayer(wxCommandEvent& ev);
    int getSelectedLayerId();
};

} // namespace ui
