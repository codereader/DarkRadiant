#include "LayerControlDialog.h"

#include "i18n.h"
#include "itextstream.h"
#include "ilayer.h"
#include "ui/imainframe.h"
#include "iselection.h"

#include "registry/registry.h"

#include <wx/button.h>
#include <wx/bmpbuttn.h>
#include <wx/tglbtn.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/artprov.h>
#include <functional>

#include "wxutil/ScrollWindow.h"
#include "wxutil/Button.h"

#include "scene/LayerUsageBreakdown.h"

#include <map>

namespace ui
{

namespace
{
	const std::string RKEY_ROOT = "user/ui/layers/controlDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

LayerControlDialog::LayerControlDialog() :
	TransientWindow(_("Layers"), GlobalMainFrame().getWxTopLevelWindow(), true),
	_dialogPanel(nullptr),
	_controlContainer(nullptr),
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
	auto dialogPanel = new wxutil::ScrollWindow(this, wxID_ANY);
	dialogPanel->SetShouldScrollToChildOnFocus(false);
	dialogPanel->SetScrollRate(0, 15);

	_dialogPanel = dialogPanel;
	
	_dialogPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	_controlContainer = new wxFlexGridSizer(1, 4, 3, 3);
	_controlContainer->AddGrowableCol(2);

    _dialogPanel->GetSizer()->Add(_controlContainer, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 12);

	// Add the option buttons ("Create Layer", etc.) to the window
	createButtons();

	_dialogPanel->FitInside(); // ask the sizer about the needed size
}

void LayerControlDialog::createButtons()
{
	// Show all / hide all buttons
	wxBoxSizer* hideShowBox = new wxBoxSizer(wxHORIZONTAL);

	_showAllLayers = new wxButton(_dialogPanel, wxID_ANY, _("Show all"));
	_hideAllLayers = new wxButton(_dialogPanel, wxID_ANY, _("Hide all"));

	_showAllLayers->Bind(wxEVT_BUTTON, &LayerControlDialog::onShowAllLayers, this);
	_hideAllLayers->Bind(wxEVT_BUTTON, &LayerControlDialog::onHideAllLayers, this);

	// Create layer button
	wxButton* createButton = new wxButton(_dialogPanel, wxID_ANY, _("New"));
	createButton->SetBitmap(wxArtProvider::GetBitmap(wxART_PLUS));

	wxutil::button::connectToCommand(createButton, "CreateNewLayerDialog");

	hideShowBox->Add(_showAllLayers, 1, wxEXPAND | wxTOP, 6);
	hideShowBox->Add(_hideAllLayers, 1, wxEXPAND | wxLEFT | wxTOP, 6);

    _dialogPanel->GetSizer()->Add(createButton, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 12);
    _dialogPanel->GetSizer()->Add(hideShowBox, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 12);
}

void LayerControlDialog::clearControls()
{
	// Delete all wxWidgets objects first
	_controlContainer->Clear(true);

	// Remove all previously allocated layercontrols
	_layerControls.clear();
}

void LayerControlDialog::refresh()
{
	clearControls();

	if (!GlobalMapModule().getRoot())
	{
		return; // no map present, don't add any layer controls
	}

    std::map<std::string, LayerControlPtr> sortedControls;

	// Traverse the layers
	GlobalMapModule().getRoot()->getLayerManager().foreachLayer([&](int layerID, const std::string& layerName)
    {
        // Create a new layercontrol for each visited layer
        // Store the object in a sorted container
        sortedControls[layerName] = std::make_shared<LayerControl>(_dialogPanel, layerID);
    });

    // Assign all controls to the target vector, alphabetically sorted
    for (std::pair<std::string, LayerControlPtr> pair : sortedControls)
    {
        _layerControls.push_back(pair.second);
    }

	_controlContainer->SetRows(static_cast<int>(_layerControls.size()));

	for (const LayerControlPtr& control : _layerControls)
	{
		_controlContainer->Add(control->getToggle(), 0);
		_controlContainer->Add(control->getStatusWidget(), 0, wxEXPAND | wxTOP | wxBOTTOM, 1);
		_controlContainer->Add(control->getLabelButton(), 0, wxEXPAND);
		_controlContainer->Add(control->getButtons(), 0, wxEXPAND);

        if (control == _layerControls.front())
        {
            // Prevent setting the focus on the buttons at the bottom which lets the scrollbar 
            // of the window jump around (#4089), set the focus on the first button.
			control->getLabelButton()->SetFocus();
        }
	}

	_controlContainer->Layout();
	_dialogPanel->FitInside(); // ask the sizer about the needed size

	update();
}

void LayerControlDialog::update()
{
	if (!GlobalMapModule().getRoot())
	{
		return;
	}

	auto& layerSystem = GlobalMapModule().getRoot()->getLayerManager();

	// Broadcast the update() call
	for (const LayerControlPtr& control : _layerControls)
	{
		control->update();
	}

	// Update usage next time round
	_rescanSelectionOnIdle = true;

	// Update the show/hide all button sensitiveness
    std::size_t numVisible = 0;
    std::size_t numHidden = 0;

	layerSystem.foreachLayer([&](int layerID, const std::string& layerName)
    {
        if (layerSystem.layerIsVisible(layerID))
        {
            numVisible++;
        }
        else
        {
            numHidden++;
        }
    });

	_showAllLayers->Enable(numHidden > 0);
	_hideAllLayers->Enable(numVisible > 0);
}

void LayerControlDialog::updateLayerUsage()
{
	_rescanSelectionOnIdle = false;

	// Scan the selection and get the histogram
	scene::LayerUsageBreakdown breakDown = scene::LayerUsageBreakdown::CreateFromSelection();

	for (const LayerControlPtr& control : _layerControls)
	{
		assert(static_cast<int>(breakDown.size()) > control->getLayerId());

		control->updateUsageStatusWidget(breakDown[control->getLayerId()]);
	}
}

void LayerControlDialog::onIdle()
{
	if (!_rescanSelectionOnIdle) return;

	updateLayerUsage();
}

void LayerControlDialog::toggle(const cmd::ArgumentList& args)
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

LayerControlDialogPtr& LayerControlDialog::InstancePtr()
{
	static LayerControlDialogPtr _instancePtr;
	return _instancePtr;
}

LayerControlDialog& LayerControlDialog::Instance()
{
	LayerControlDialogPtr& instancePtr = InstancePtr();

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

// TransientWindow callbacks
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

} // namespace ui
