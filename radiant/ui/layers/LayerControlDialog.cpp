#include "LayerControlDialog.h"

#include "i18n.h"
#include "itextstream.h"
#include "ilayer.h"
#include "ui/imainframe.h"
#include "iselection.h"

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

namespace ui
{

namespace
{
	const std::string RKEY_ROOT = "user/ui/layers/controlDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

LayerControlDialog::LayerControlDialog() :
	TransientWindow(_("Layers"), GlobalMainFrame().getWxTopLevelWindow(), true),
    _layersView(nullptr),
	_showAllLayers(nullptr),
	_hideAllLayers(nullptr),
	_refreshTreeOnIdle(false),
	_updateTreeOnIdle(false),
	_rescanSelectionOnIdle(false)
{
	populateWindow();

	Bind(wxEVT_IDLE, [&](wxIdleEvent&) { onIdle(); });

	InitialiseWindowPosition(230, 400, RKEY_WINDOW_STATE);
    SetMinClientSize(wxSize(230, 200));
}

void LayerControlDialog::populateWindow()
{
    _layerStore = new wxutil::TreeModel(_columns);
    _layersView = wxutil::TreeView::CreateWithModel(this, _layerStore.get(), wxDV_SINGLE | wxDV_NO_HEADER);

    _layersView->AppendToggleColumn(_("Visible"), _columns.visible.getColumnIndex(),
        wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
    _layersView->AppendColumn(new wxutil::IndicatorColumn(_("Contains Selection"),
        _columns.selectionIsPartOfLayer.getColumnIndex()));
    _layersView->AppendTextColumn("", _columns.name.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    _layersView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &LayerControlDialog::onItemActivated, this);
    _layersView->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &LayerControlDialog::onItemToggled, this);
    _layersView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &LayerControlDialog::onItemSelected, this);

    // Configure drag and drop
    _layersView->EnableDragSource(wxDF_UNICODETEXT);
    _layersView->EnableDropTarget(wxDF_UNICODETEXT);
    _layersView->Bind(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, &LayerControlDialog::onBeginDrag, this);
    _layersView->Bind(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, &LayerControlDialog::onDropPossible, this);
    _layersView->Bind(wxEVT_DATAVIEW_ITEM_DROP, &LayerControlDialog::onDrop, this);

    SetSizer(new wxBoxSizer(wxVERTICAL));

    auto overallVBox = new wxBoxSizer(wxVERTICAL);
    GetSizer()->Add(overallVBox, 1, wxEXPAND | wxALL, 12);

    overallVBox->Add(_layersView, 1, wxEXPAND);

	// Add the option buttons ("Create Layer", etc.) to the window
	createButtons();
}

void LayerControlDialog::createButtons()
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
    _deleteButton->Bind(wxEVT_BUTTON, &LayerControlDialog::onDeleteLayer, this);
    _deleteButton->SetToolTip(_("Delete this layer"));

    _renameButton = new wxBitmapButton(this, wxID_ANY, wxutil::GetLocalBitmap("edit.png"));
    _renameButton->Bind(wxEVT_BUTTON, &LayerControlDialog::onRenameLayer, this);
    _renameButton->SetToolTip(_("Rename this layer"));

    topRow->Add(createButton, 1, wxEXPAND | wxRIGHT, 6);
    topRow->Add(_renameButton, 0, wxEXPAND | wxRIGHT, 6);
    topRow->Add(_deleteButton, 0, wxEXPAND, 6);

	hideShowBox->Add(_showAllLayers, 1, wxEXPAND | wxTOP, 6);
	hideShowBox->Add(_hideAllLayers, 1, wxEXPAND | wxLEFT | wxTOP, 6);

    GetSizer()->Add(topRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    GetSizer()->Add(hideShowBox, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 12);
}

void LayerControlDialog::clearControls()
{
    _layerItemMap.clear();
    _layerStore->Clear();
}

void LayerControlDialog::queueRefresh()
{
    _refreshTreeOnIdle = true;
    _rescanSelectionOnIdle = true;
}

void LayerControlDialog::queueUpdate()
{
    _updateTreeOnIdle = true;
    _rescanSelectionOnIdle = true;
}

void LayerControlDialog::refresh()
{
    _refreshTreeOnIdle = false;

	clearControls();

    if (!GlobalMapModule().getRoot()) return; // no map present

	// Traverse the layers
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto activeLayerId = layerManager.getActiveLayer();

    // Update the show/hide all button sensitiveness
    std::size_t numVisible = 0;
    std::size_t numHidden = 0;

    layerManager.foreachLayer([&](int layerId, const std::string& layerName)
    {
        // Store the object in a sorted container
        auto row = _layerStore->AddItem();

        row[_columns.id] = layerId;
        row[_columns.name] = layerName;

        if (activeLayerId == layerId)
        {
            row[_columns.name] = wxutil::TreeViewItemStyle::ActiveItemStyle();
        }

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

        row.SendItemAdded();

        _layerItemMap.emplace(layerId, row.getItem());
    });

    updateButtonSensitivity(numVisible, numHidden);
}

void LayerControlDialog::update()
{
    _updateTreeOnIdle = false;

	if (!GlobalMapModule().getRoot())
	{
		return;
	}

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

        row[_columns.name] = activeLayerId == layerId ? 
            wxutil::TreeViewItemStyle::ActiveItemStyle() : wxDataViewItemAttr(); // no style

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

void LayerControlDialog::updateButtonSensitivity(std::size_t numVisible, std::size_t numHidden)
{
	_showAllLayers->Enable(numHidden > 0);
	_hideAllLayers->Enable(numVisible > 0);

    updateItemActionSensitivity();
}

void LayerControlDialog::updateItemActionSensitivity()
{
    auto selectedLayerId = getSelectedLayerId();

    // Don't allow deleting or renaming the default layer (or -1)
    _deleteButton->Enable(selectedLayerId > 0);
    _renameButton->Enable(selectedLayerId > 0);
}


void LayerControlDialog::updateLayerUsage()
{
	_rescanSelectionOnIdle = false;

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

void LayerControlDialog::onIdle()
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

void LayerControlDialog::ToggleDialog(const cmd::ArgumentList& args)
{
	Instance().ToggleVisibility();
}

void LayerControlDialog::onMainFrameConstructed()
{
	// Lookup the stored window information in the registry
	if (GlobalRegistry().getAttribute(RKEY_WINDOW_STATE, "visible") == "1")
	{
		// Show dialog
		Instance().Show();
	}
}

void LayerControlDialog::onMainFrameShuttingDown()
{
	rMessage() << "LayerControlDialog shutting down." << std::endl;

	// Write the visibility status to the registry
	GlobalRegistry().setAttribute(RKEY_WINDOW_STATE, "visible", IsShownOnScreen() ? "1" : "0");

    // Hide the window and save its state
    if (IsShownOnScreen())
    {
        Hide();
    }

	// Destroy the window
	SendDestroyEvent();

	// Final step: clear the instance pointer
	InstancePtr().reset();
}

std::shared_ptr<LayerControlDialog>& LayerControlDialog::InstancePtr()
{
	static std::shared_ptr<LayerControlDialog> _instancePtr;
	return _instancePtr;
}

LayerControlDialog& LayerControlDialog::Instance()
{
	auto& instancePtr = InstancePtr();

	if (!instancePtr)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new LayerControlDialog);

		// Pre-destruction cleanup
		GlobalMainFrame().signal_MainFrameShuttingDown().connect(
            sigc::mem_fun(*instancePtr, &LayerControlDialog::onMainFrameShuttingDown));
	}

	return *instancePtr;
}

void LayerControlDialog::_preShow()
{
	TransientWindow::_preShow();

	_selectionChangedSignal = GlobalSelectionSystem().signal_selectionChanged().connect([this](const ISelectable&)
	{
		_rescanSelectionOnIdle = true;
	});

	connectToMapRoot();

	_mapEventSignal = GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(this, &LayerControlDialog::onMapEvent)
	);

	// Re-populate the dialog
	queueRefresh();
}

void LayerControlDialog::_postHide()
{
	disconnectFromMapRoot();

	_mapEventSignal.disconnect();
	_selectionChangedSignal.disconnect();
	_refreshTreeOnIdle = false;
    _updateTreeOnIdle = false;
	_rescanSelectionOnIdle = false;
}

void LayerControlDialog::connectToMapRoot()
{
	// Always disconnect first
	disconnectFromMapRoot();

	if (GlobalMapModule().getRoot())
	{
		auto& layerSystem = GlobalMapModule().getRoot()->getLayerManager();

		// Layer creation/addition/removal triggers a refresh
		_layersChangedSignal = layerSystem.signal_layersChanged().connect(
			sigc::mem_fun(this, &LayerControlDialog::queueRefresh));

		// Visibility change doesn't repopulate the dialog
		_layerVisibilityChangedSignal = layerSystem.signal_layerVisibilityChanged().connect(
			sigc::mem_fun(this, &LayerControlDialog::queueUpdate));

		// Node membership triggers a selection rescan
		_nodeLayerMembershipChangedSignal = layerSystem.signal_nodeMembershipChanged().connect([this]()
		{
			_rescanSelectionOnIdle = true;
		});
	}
}

void LayerControlDialog::disconnectFromMapRoot()
{
	_nodeLayerMembershipChangedSignal.disconnect();
	_layersChangedSignal.disconnect();
	_layerVisibilityChangedSignal.disconnect();
}

void LayerControlDialog::setVisibilityOfAllLayers(bool visible)
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

void LayerControlDialog::onMapEvent(IMap::MapEvent ev)
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

int LayerControlDialog::getSelectedLayerId()
{
    auto item = _layersView->GetSelection();

    if (!item.IsOk()) return -1;

    wxutil::TreeModel::Row row(item, *_layerStore);

    return row[_columns.id].getInteger();
}

void LayerControlDialog::onItemActivated(wxDataViewEvent& ev)
{
    // Don't react to double-clicks on the checkbox
    if (ev.GetColumn() == _columns.visible.getColumnIndex())
    {
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

void LayerControlDialog::onItemToggled(wxDataViewEvent& ev)
{
    if (!GlobalMapModule().getRoot()) return;

    if (ev.GetDataViewColumn() != nullptr &&
        static_cast<int>(ev.GetDataViewColumn()->GetModelColumn()) == _columns.visible.getColumnIndex())
    {
        // Model value in the boolean column has changed, this means the checkbox has been toggled
        wxutil::TreeModel::Row row(ev.GetItem(), *_layerStore);

        GlobalMapModule().getRoot()->getLayerManager().setLayerVisibility(row[_columns.id].getInteger(), row[_columns.visible].getBool());
    }
}

void LayerControlDialog::onItemSelected(wxDataViewEvent& ev)
{
    updateItemActionSensitivity();
}

void LayerControlDialog::onRenameLayer(wxCommandEvent& ev)
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

void LayerControlDialog::onDeleteLayer(wxCommandEvent& ev)
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

void LayerControlDialog::onBeginDrag(wxDataViewEvent& ev)
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

void LayerControlDialog::onDropPossible(wxDataViewEvent& ev)
{
    
}

void LayerControlDialog::onDrop(wxDataViewEvent& ev)
{
    if (!GlobalMapModule().getRoot())
    {
        rError() << "Can't change layer hierarchy, no map root present" << std::endl;
        return;
    }

    wxDataViewItem item(ev.GetItem());
    wxutil::TreeModel::Row row(item, *_layerStore);

    auto targetLayerId = row[_columns.id].getInteger();

    // Read the source layer ID and veto the event if it's the same as the source ID
    if (auto obj = dynamic_cast<wxTextDataObject*>(ev.GetDataObject()); obj)
    {
        auto sourceLayerId = string::convert<int>(obj->GetText().ToStdString(), -1);

        if (sourceLayerId == targetLayerId)
        {
            ev.Veto();
            return;
        }

        rMessage() << "Assigning layer " << sourceLayerId << " to parent layer " << targetLayerId << std::endl;

        auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
        layerManager.setParentLayer(sourceLayerId, targetLayerId);
    }
}

} // namespace ui
