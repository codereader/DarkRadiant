#include "LayerControlPanel.h"

#include "i18n.h"
#include "itextstream.h"
#include "ilayer.h"
#include "iselection.h"

#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/tglbtn.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/artprov.h>
#include <wx/dataobj.h>

#include "wxutil/Button.h"

#include "scene/LayerUsageBreakdown.h"
#include "wxutil/Bitmap.h"
#include "wxutil/EntryAbortedException.h"
#include "wxutil/dataview/IndicatorColumn.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "wxutil/dialog/Dialog.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/menu/IconTextMenuItem.h"

namespace ui
{

LayerControlPanel::LayerControlPanel(wxWindow* parent) :
    DockablePanel(parent),
    _layersView(nullptr),
	_showAllLayers(nullptr),
	_hideAllLayers(nullptr),
	_refreshTreeOnIdle(false),
	_updateTreeOnIdle(false),
	_rescanSelectionOnIdle(false)
{
	populateWindow();
}

LayerControlPanel::~LayerControlPanel()
{
    if (panelIsActive())
    {
        disconnectListeners();
    }
}

void LayerControlPanel::onPanelActivated()
{
    connectListeners();
    queueRefresh();
}

void LayerControlPanel::onPanelDeactivated()
{
    disconnectListeners();
}

void LayerControlPanel::connectListeners()
{
    _selectionChangedSignal = GlobalSelectionSystem().signal_selectionChanged().connect([this](const ISelectable&)
    {
        _rescanSelectionOnIdle = true;
        requestIdleCallback();
    });

    connectToMapRoot();

    _mapEventSignal = GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(this, &LayerControlPanel::onMapEvent)
    );
}

void LayerControlPanel::disconnectListeners()
{
    disconnectFromMapRoot();

    _mapEventSignal.disconnect();
    _selectionChangedSignal.disconnect();

    _refreshTreeOnIdle = false;
    _updateTreeOnIdle = false;
    _rescanSelectionOnIdle = false;
}

void LayerControlPanel::populateWindow()
{
    _layerStore = new wxutil::TreeModel(_columns);
    _layersView = wxutil::TreeView::CreateWithModel(this, _layerStore.get(), wxDV_SINGLE | wxDV_NO_HEADER);

    _layersView->AppendToggleColumn(_("Visible"), _columns.visible.getColumnIndex(),
        wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
    _layersView->AppendColumn(new wxutil::IndicatorColumn(_("Contains Selection"),
        _columns.selectionIsPartOfLayer.getColumnIndex()));
    _layersView->AppendTextColumn("", _columns.name.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

    _layersView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &LayerControlPanel::onItemActivated, this);
    _layersView->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &LayerControlPanel::onItemValueChanged, this);
    _layersView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &LayerControlPanel::onItemSelected, this);

    // Configure drag and drop
    _layersView->EnableDragSource(wxDF_UNICODETEXT);
    _layersView->EnableDropTarget(wxDF_UNICODETEXT);
    _layersView->Bind(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, &LayerControlPanel::onBeginDrag, this);
    _layersView->Bind(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, &LayerControlPanel::onDropPossible, this);
    _layersView->Bind(wxEVT_DATAVIEW_ITEM_DROP, &LayerControlPanel::onDrop, this);
    _layersView->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, [&](auto&) { _popupMenu->show(this); });

    SetSizer(new wxBoxSizer(wxVERTICAL));

    auto overallVBox = new wxBoxSizer(wxVERTICAL);
    GetSizer()->Add(overallVBox, 1, wxEXPAND | wxALL, 12);

    overallVBox->Add(_layersView, 1, wxEXPAND);

	// Add the option buttons ("Create Layer", etc.) to the window
	createButtons();
    createPopupMenu();
}

void LayerControlPanel::createButtons()
{
	// Show all / hide all buttons
	auto hideShowBox = new wxBoxSizer(wxHORIZONTAL);

	_showAllLayers = new wxButton(this, wxID_ANY, _("Show all"));
	_hideAllLayers = new wxButton(this, wxID_ANY, _("Hide all"));

    _showAllLayers->Bind(wxEVT_BUTTON, [this](auto& ev) { setVisibilityOfAllLayers(true); });
	_hideAllLayers->Bind(wxEVT_BUTTON, [this](auto& ev) { setVisibilityOfAllLayers(false); });

	// Create layer button
    auto topRow = new wxBoxSizer(wxHORIZONTAL);

	auto createButton = new wxButton(this, wxID_ANY, _("New"));
	createButton->SetBitmap(wxArtProvider::GetBitmap(wxART_PLUS));
	wxutil::button::connectToCommand(createButton, "CreateNewLayerDialog");

    _deleteButton = new wxBitmapButton(this, wxID_ANY, wxutil::GetLocalBitmap("delete.png"));
    _deleteButton->Bind(wxEVT_BUTTON, [this](auto&) { deleteSelectedLayer(); });
    _deleteButton->SetToolTip(_("Delete the selected layer"));

    _renameButton = new wxBitmapButton(this, wxID_ANY, wxutil::GetLocalBitmap("edit.png"));
    _renameButton->Bind(wxEVT_BUTTON, [this](auto&) { renameSelectedLayer(); });
    _renameButton->SetToolTip(_("Rename the selected layer"));

    topRow->Add(createButton, 1, wxEXPAND | wxRIGHT, 6);
    topRow->Add(_renameButton, 0, wxEXPAND | wxRIGHT, 6);
    topRow->Add(_deleteButton, 0, wxEXPAND, 6);

	hideShowBox->Add(_showAllLayers, 1, wxEXPAND | wxTOP, 6);
	hideShowBox->Add(_hideAllLayers, 1, wxEXPAND | wxLEFT | wxTOP, 6);

    GetSizer()->Add(topRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    GetSizer()->Add(hideShowBox, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 12);
}

void LayerControlPanel::createPopupMenu()
{
    _popupMenu = std::make_shared<wxutil::PopupMenu>();

    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Select all items in this hierarchy\tDoubleclick"), ""),
        [this]() { GlobalMapModule().getRoot()->getLayerManager().setSelected(getSelectedLayerId(), true); },
        [this]() { return getSelectedLayerId() != -1; }
    );
    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("De-select all items in this hierarchy\tSHIFT-Doublelick"), ""),
        [this]() { GlobalMapModule().getRoot()->getLayerManager().setSelected(getSelectedLayerId(), false); },
        [this]() { return getSelectedLayerId() != -1; }
    );
    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Make this the active layer\tCTRL-Doubleclick"), ""),
        [this]() { GlobalMapModule().getRoot()->getLayerManager().setActiveLayer(getSelectedLayerId()); queueUpdate(); },
        [this]() { return getSelectedLayerId() != -1; }
    );
    _popupMenu->addSeparator();
    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Make layer top-level"), ""),
        [this]() { GlobalMapModule().getRoot()->getLayerManager().setParentLayer(getSelectedLayerId(), -1); },
        [this]() { return GlobalMapModule().getRoot()->getLayerManager().getParentLayer(getSelectedLayerId()) != -1; }
    );
    _popupMenu->addSeparator();
    _popupMenu->addItem(
        new wxutil::IconTextMenuItem(_("Rename this layer"), "edit.png"),
        [this]() { renameSelectedLayer(); },
        [this]() { return getSelectedLayerId() > 0; } // must not be the default layer
    );
    _popupMenu->addItem(
        new wxutil::IconTextMenuItem(_("Delete this layer"), "delete.png"),
        [this]() { deleteSelectedLayer(); },
        [this]() { return getSelectedLayerId() > 0; } // must not be the default layer
    );
}

void LayerControlPanel::clearControls()
{
    _layerItemMap.clear();
    _layerStore->Clear();
}

void LayerControlPanel::queueRefresh()
{
    _refreshTreeOnIdle = true;
    _rescanSelectionOnIdle = true;
    requestIdleCallback();
}

void LayerControlPanel::queueUpdate()
{
    _updateTreeOnIdle = true;
    _rescanSelectionOnIdle = true;
    requestIdleCallback();
}

class LayerControlPanel::TreePopulator
{
private:
    const TreeColumns& _columns;
    wxutil::TreeModel::Ptr _layerStore;

    std::map<int, wxDataViewItem> _layerItemMap;

    scene::ILayerManager& _layerManager;;
    int _activeLayerId;

    std::size_t _numVisible = 0;
    std::size_t _numHidden = 0;

public:
    TreePopulator(const wxutil::TreeModel::Ptr& layerStore, const TreeColumns& columns) :
        _columns(columns),
        _layerStore(new wxutil::TreeModel(_columns)),
        _layerManager(GlobalMapModule().getRoot()->getLayerManager()),
        _activeLayerId(_layerManager.getActiveLayer())
    {}

    std::size_t getNumVisibleLayers() const
    {
        return _numVisible;
    }

    std::size_t getNumHiddenLayers() const
    {
        return _numHidden;
    }

    std::map<int, wxDataViewItem>& getLayerItemMap()
    {
        return _layerItemMap;
    }

    const wxutil::TreeModel::Ptr& getLayerStore() const
    {
        return _layerStore;
    }

    wxDataViewItem processLayer(int layerId, const std::string& layerName)
    {
        auto existingItem = _layerItemMap.find(layerId);

        if (existingItem != _layerItemMap.end()) return existingItem->second;

        // Find the parent ID of this layer
        auto parentLayerId = _layerManager.getParentLayer(layerId);

        auto parentItem = parentLayerId == -1 ? wxDataViewItem() :
            processLayer(parentLayerId, _layerManager.getLayerName(parentLayerId));

        // Parent item located, insert this layer as child element
        auto row = _layerStore->AddItemUnderParent(parentItem);

        row[_columns.id] = layerId;
        row[_columns.name] = layerName;

        if (_activeLayerId == layerId)
        {
            row[_columns.name].setAttr(wxutil::TreeViewItemStyle::ActiveItemStyle());
        }

        if (_layerManager.layerIsVisible(layerId))
        {
            row[_columns.visible] = true;
            _numVisible++;
        }
        else
        {
            row[_columns.visible] = false;
            _numHidden++;
        }

        row.SendItemAdded();

        _layerItemMap.emplace(layerId, row.getItem());

        return row.getItem();
    }
};

void LayerControlPanel::refresh()
{
    _refreshTreeOnIdle = false;

    // Find out which layers are currently expanded and/or selected
    auto selectedLayerId = getSelectedLayerId();
    std::set<int> expandedLayers;

    for (const auto& [id, item] : _layerItemMap)
    {
        if (_layersView->IsExpanded(item))
        {
            expandedLayers.insert(id);
        }
    }

	clearControls();

    if (!GlobalMapModule().getRoot()) return; // no map present

	// Traverse the layers
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    TreePopulator populator(_layerStore, _columns);
    layerManager.foreachLayer(
        std::bind(&TreePopulator::processLayer, &populator, std::placeholders::_1, std::placeholders::_2));

    // Get the model and sort each hierarchy level by name
    _layerStore = populator.getLayerStore();
    _layerStore->SortModelByColumn(_columns.name);

    // Now associate the layer store with our tree view
    _layersView->AssociateModel(_layerStore.get());

    _layerItemMap = std::move(populator.getLayerItemMap());

    // Restore the expanded state of tree elements
    for (const auto& [id, item] : _layerItemMap)
    {
        if (expandedLayers.count(id) > 0)
        {
            _layersView->Expand(item);
        }

        if (selectedLayerId == id)
        {
            _layersView->Select(item);
            _layersView->EnsureVisible(item);
        }
    }

    updateButtonSensitivity(populator.getNumVisibleLayers(), populator.getNumHiddenLayers());
}

void LayerControlPanel::update()
{
    _updateTreeOnIdle = false;

    if (!GlobalMapModule().getRoot()) return;

	auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto activeLayerId = layerManager.getActiveLayer();

	// Update the show/hide all button sensitiveness
    std::size_t numVisible = 0;
    std::size_t numHidden = 0;

    layerManager.foreachLayer([&](int layerId, const std::string& layerName)
    {
        auto existingItem = _layerItemMap.find(layerId);

        if (existingItem == _layerItemMap.end()) return; // tracking error?

        wxutil::TreeModel::Row row(existingItem->second, *_layerStore);

        row[_columns.name] = layerName;

        row[_columns.name].setAttr(
            activeLayerId == layerId ? wxutil::TreeViewItemStyle::ActiveItemStyle()
                                     : wxDataViewItemAttr() // no style
        );

        if (layerManager.layerIsVisible(layerId))
        {
            row[_columns.visible] = true;
            numVisible++;
        }
        else
        {
            row[_columns.visible] = false;
            numHidden++;
        }

        row.SendItemChanged();
    });

    updateButtonSensitivity(numVisible, numHidden);
}

void LayerControlPanel::updateButtonSensitivity(std::size_t numVisible, std::size_t numHidden)
{
	_showAllLayers->Enable(numHidden > 0);
	_hideAllLayers->Enable(numVisible > 0);

    updateItemActionSensitivity();
}

void LayerControlPanel::updateItemActionSensitivity()
{
    auto selectedLayerId = getSelectedLayerId();

    // Don't allow deleting or renaming the default layer (or -1)
    _deleteButton->Enable(selectedLayerId > 0);
    _renameButton->Enable(selectedLayerId > 0);
}


void LayerControlPanel::updateLayerUsage()
{
	_rescanSelectionOnIdle = false;

    if (!GlobalMapModule().getRoot()) return;

    // Scan the selection and get the histogram
	auto breakDown = scene::LayerUsageBreakdown::CreateFromSelection();

    GlobalMapModule().getRoot()->getLayerManager().foreachLayer([&](int layerId, const std::string& layerName)
    {
        auto existingItem = _layerItemMap.find(layerId);

        if (existingItem == _layerItemMap.end()) return; // tracking error?

        wxutil::TreeModel::Row row(existingItem->second, *_layerStore);

        row[_columns.selectionIsPartOfLayer] = breakDown[layerId] > 0;

        row.SendItemChanged();
    });
}

void LayerControlPanel::onIdle()
{
    if (_refreshTreeOnIdle)
    {
        refresh();
    }

    if (_updateTreeOnIdle)
    {
        update();
    }

	if (_rescanSelectionOnIdle)
	{
	    updateLayerUsage();
	}
}

void LayerControlPanel::connectToMapRoot()
{
	// Always disconnect first
	disconnectFromMapRoot();

	if (GlobalMapModule().getRoot())
	{
		auto& layerSystem = GlobalMapModule().getRoot()->getLayerManager();

		// Layer creation/addition/removal triggers a refresh
		_layersChangedSignal = layerSystem.signal_layersChanged().connect(
			sigc::mem_fun(this, &LayerControlPanel::queueRefresh));

		// Visibility change doesn't repopulate the dialog
		_layerVisibilityChangedSignal = layerSystem.signal_layerVisibilityChanged().connect(
			sigc::mem_fun(this, &LayerControlPanel::queueUpdate));

        // Hierarchy changes trigger a tree rebuild
        _layerHierarchyChangedSignal = layerSystem.signal_layerHierarchyChanged().connect(
            sigc::mem_fun(this, &LayerControlPanel::queueRefresh));

		// Node membership triggers a selection rescan
		_nodeLayerMembershipChangedSignal = layerSystem.signal_nodeMembershipChanged().connect([this]()
		{
			_rescanSelectionOnIdle = true;
            requestIdleCallback();
		});
	}
}

void LayerControlPanel::disconnectFromMapRoot()
{
	_nodeLayerMembershipChangedSignal.disconnect();
    _layerHierarchyChangedSignal.disconnect();
	_layersChangedSignal.disconnect();
	_layerVisibilityChangedSignal.disconnect();
}

void LayerControlPanel::setVisibilityOfAllLayers(bool visible)
{
	if (!GlobalMapModule().getRoot())
	{
		rError() << "Can't change layer visibility, no map loaded." << std::endl;
		return;
	}

	auto& layerSystem = GlobalMapModule().getRoot()->getLayerManager();

	layerSystem.foreachLayer([&](int layerID, const std::string&)
    {
		layerSystem.setLayerVisibility(layerID, visible);
    });
}

void LayerControlPanel::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapLoaded)
	{
		// Rebuild the dialog once a map is loaded
		connectToMapRoot();
		queueRefresh();
	}
	else if (ev == IMap::MapUnloading)
	{
		disconnectFromMapRoot();
		clearControls();
	}
}

int LayerControlPanel::getSelectedLayerId()
{
    auto item = _layersView->GetSelection();

    if (!item.IsOk()) return -1;

    wxutil::TreeModel::Row row(item, *_layerStore);

    return row[_columns.id].getInteger();
}

void LayerControlPanel::onItemActivated(wxDataViewEvent& ev)
{
    // When double-clicking the checkbox, just toggle the state
    if (ev.GetColumn() == _columns.visible.getColumnIndex())
    {
        wxutil::TreeModel::Row row(ev.GetItem(), *_layerStore);

        // Invert the visibility
        auto visible = !row[_columns.visible].getBool();
        row[_columns.visible] = visible;
        row.SendItemChanged();
        return;
    }

    if (!GlobalMapModule().getRoot())
    {
        rError() << "Can't select layer, no map root present" << std::endl;
        return;
    }

    auto layerId = getSelectedLayerId();

    // When holding down CTRL the user sets this as active
    if (wxGetKeyState(WXK_CONTROL))
    {
        GlobalMapModule().getRoot()->getLayerManager().setActiveLayer(layerId);
        queueRefresh();
        return;
    }

    // By default, we SELECT the layer
    // The user can choose to DESELECT the layer when holding down shift
    bool selected = !wxGetKeyState(WXK_SHIFT);

    // Set the entire layer to selected
    GlobalMapModule().getRoot()->getLayerManager().setSelected(layerId, selected);
}

void LayerControlPanel::onItemValueChanged(wxDataViewEvent& ev)
{
    auto root = GlobalMapModule().getRoot();
    if (!root) return;

    // In wxGTK the column shipped with the even doesn't seem to be correct
    // So we just check if the value in the model differs from the layer's visibility
    
    wxutil::TreeModel::Row row(ev.GetItem(), *_layerStore);

    auto visible = row[_columns.visible].getBool();
    auto layerId = row[_columns.id].getInteger();

    // Check if the visibility has changed in the model
    if (root->getLayerManager().layerIsVisible(layerId) != visible)
    {
        root->getLayerManager().setLayerVisibility(layerId, visible);
    }
}

void LayerControlPanel::onItemSelected(wxDataViewEvent& ev)
{
    updateItemActionSensitivity();
}

void LayerControlPanel::renameSelectedLayer()
{
    if (!GlobalMapModule().getRoot())
    {
        rError() << "Can't rename layer, no map root present" << std::endl;
        return;
    }

    auto selectedLayerId = getSelectedLayerId();
    auto& layerSystem = GlobalMapModule().getRoot()->getLayerManager();

    while (true)
    {
        // Query the name of the new layer from the user
        std::string newLayerName;

        try
        {
            newLayerName = wxutil::Dialog::TextEntryDialog(
                _("Rename Layer"),
                _("Enter new Layer Name"),
                layerSystem.getLayerName(selectedLayerId),
                this
            );
        }
        catch (wxutil::EntryAbortedException&)
        {
            break;
        }

        // Attempt to rename the layer, this will return -1 if the operation fails
        bool success = layerSystem.renameLayer(selectedLayerId, newLayerName);

        if (success)
        {
            // Stop here, the control might already have been destroyed
            GlobalMapModule().setModified(true);
            return;
        }
        else
        {
            // Wrong name, let the user try again
            wxutil::Messagebox::ShowError(_("Could not rename layer, please try again."));
            continue;
        }
    }
}

void LayerControlPanel::deleteSelectedLayer()
{
    if (!GlobalMapModule().getRoot())
    {
        rError() << "Can't delete layer, no map root present" << std::endl;
        return;
    }

    auto selectedLayerId = getSelectedLayerId();
    auto& layerSystem = GlobalMapModule().getRoot()->getLayerManager();

    // Ask the about the deletion
    auto msg = _("Do you really want to delete this layer?");
    msg += "\n" + layerSystem.getLayerName(selectedLayerId);

    IDialogPtr box = GlobalDialogManager().createMessageBox(
        _("Confirm Layer Deletion"), msg, IDialog::MESSAGE_ASK
    );

    if (box->run() == IDialog::RESULT_YES)
    {
        GlobalCommandSystem().executeCommand("DeleteLayer", cmd::Argument(selectedLayerId));
    }
}

void LayerControlPanel::onBeginDrag(wxDataViewEvent& ev)
{
    wxDataViewItem item(ev.GetItem());
    wxutil::TreeModel::Row row(item, *_layerStore);

    auto selectedLayerId = row[_columns.id].getInteger();

    // Don't allow rearranging the default layer
    if (selectedLayerId > 0)
    {
        auto obj = new wxTextDataObject;
        obj->SetText(string::to_string(selectedLayerId));
        ev.SetDataObject(obj);
        ev.SetDragFlags(wxDrag_AllowMove);
    }
}

void LayerControlPanel::onDropPossible(wxDataViewEvent& ev)
{
    if (!GlobalMapModule().getRoot()) return;

    auto selectedLayerId = getSelectedLayerId();

    wxDataViewItem item(ev.GetItem());
    wxutil::TreeModel::Row row(item, *_layerStore);

    auto targetLayerId = row[_columns.id].getInteger();

    // Don't allow dragging layers onto themselves or dragging the default layer
    if (targetLayerId == selectedLayerId || selectedLayerId == 0)
    {
        ev.Veto();
        return;
    }

    // Veto this change it the target layer is a child of the dragged one
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    if (layerManager.layerIsChildOf(targetLayerId, selectedLayerId))
    {
        ev.Veto();
        return;
    }
}

void LayerControlPanel::onDrop(wxDataViewEvent& ev)
{
    if (!GlobalMapModule().getRoot())
    {
        rError() << "Can't change layer hierarchy, no map root present" << std::endl;
        return;
    }

    auto item = ev.GetItem();
    wxutil::TreeModel::Row row(item, *_layerStore);

    // If an empty item is passed we're supposed to make it top-level again
    auto targetLayerId = item.IsOk() ? row[_columns.id].getInteger() : -1;

    if (ev.GetDataFormat() != wxDF_UNICODETEXT)
    {
        ev.Veto();
        return;
    }

    // Read the source layer ID and veto the event if it's the same as the source ID
    wxTextDataObject obj;
    obj.SetData(wxDF_UNICODETEXT, ev.GetDataSize(), ev.GetDataBuffer());

    auto sourceLayerId = string::convert<int>(obj.GetText().ToStdString(), -1);

    if (sourceLayerId == targetLayerId)
    {
        ev.Veto();
        return;
    }

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    if (layerManager.getParentLayer(sourceLayerId) == targetLayerId)
    {
        // Dragging a child onto its immediate parent will un-parent it
        // Redirect the operation to the target layer's' parent
        targetLayerId = layerManager.getParentLayer(targetLayerId);
    }

    rMessage() << "Assigning layer " << sourceLayerId << " to parent layer " << targetLayerId << std::endl;

    try
    {
        
        layerManager.setParentLayer(sourceLayerId, targetLayerId);
    }
    catch (const std::invalid_argument& ex)
    {
        wxutil::Messagebox::ShowError(fmt::format(_("Cannot set Parent Layer: {0}"), ex.what()), this);
    }
}

} // namespace ui
