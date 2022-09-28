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

#include "wxutil/Button.h"

#include "scene/LayerUsageBreakdown.h"
#include "wxutil/dataview/IndicatorColumn.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/dataview/TreeViewItemStyle.h"

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

	_showAllLayers->Bind(wxEVT_BUTTON, &LayerControlDialog::onShowAllLayers, this);
	_hideAllLayers->Bind(wxEVT_BUTTON, &LayerControlDialog::onHideAllLayers, this);

	// Create layer button
	auto createButton = new wxButton(this, wxID_ANY, _("New"));
	createButton->SetBitmap(wxArtProvider::GetBitmap(wxART_PLUS));

	wxutil::button::connectToCommand(createButton, "CreateNewLayerDialog");

	hideShowBox->Add(_showAllLayers, 1, wxEXPAND | wxTOP, 6);
	hideShowBox->Add(_hideAllLayers, 1, wxEXPAND | wxLEFT | wxTOP, 6);

    GetSizer()->Add(createButton, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    GetSizer()->Add(hideShowBox, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 12);
}

void LayerControlDialog::clearControls()
{
    _layerItemMap.clear();
    _layerStore->Clear();
}

void LayerControlDialog::refresh()
{
	clearControls();

	if (!GlobalMapModule().getRoot())
	{
		return; // no map present
	}

	// Traverse the layers
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto activeLayerId = layerManager.getActiveLayer();

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

        row[_columns.visible] = layerManager.layerIsVisible(layerId);

        row.SendItemAdded();

        _layerItemMap.emplace(layerId, row.getItem());
    });

	update();
}

void LayerControlDialog::update()
{
	if (!GlobalMapModule().getRoot())
	{
		return;
	}

	auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto activeLayerId = layerManager.getActiveLayer();

	// Update usage next time round
	_rescanSelectionOnIdle = true;

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

	_showAllLayers->Enable(numHidden > 0);
	_hideAllLayers->Enable(numVisible > 0);
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
	if (!_rescanSelectionOnIdle) return;

	updateLayerUsage();
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
	refresh();
}

void LayerControlDialog::_postHide()
{
	disconnectFromMapRoot();

	_mapEventSignal.disconnect();
	_selectionChangedSignal.disconnect();
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
			sigc::mem_fun(this, &LayerControlDialog::refresh));

		// Visibility change doesn't repopulate the dialog
		_layerVisibilityChangedSignal = layerSystem.signal_layerVisibilityChanged().connect(
			sigc::mem_fun(this, &LayerControlDialog::update));

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

void LayerControlDialog::onShowAllLayers(wxCommandEvent& ev)
{
	if (!GlobalMapModule().getRoot())
	{
		rError() << "Can't show layers, no map loaded." << std::endl;
		return;
	}

	auto& layerSystem = GlobalMapModule().getRoot()->getLayerManager();

	layerSystem.foreachLayer([&](int layerID, const std::string& layerName)
    {
		layerSystem.setLayerVisibility(layerID, true);
    });
}

void LayerControlDialog::onHideAllLayers(wxCommandEvent& ev)
{
	auto& layerSystem = GlobalMapModule().getRoot()->getLayerManager();

	layerSystem.foreachLayer([&](int layerID, const std::string& layerName)
    {
		layerSystem.setLayerVisibility(layerID, false);
    });
}

void LayerControlDialog::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapLoaded)
	{
		// Rebuild the dialog once a map is loaded
		connectToMapRoot();
		refresh();
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
        refresh();
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

} // namespace ui
