#include "AasControlDialog.h"

#include "i18n.h"
#include "iaasfile.h"
#include "iradiant.h"
#include "imainframe.h"
#include "itextstream.h"
#include "ieventmanager.h"

#include <wx/button.h>
#include <wx/tglbtn.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>

#include "registry/Widgets.h"
#include "map/Map.h"
#include "map/RenderableAasFile.h"

namespace ui
{

namespace
{
	const std::string RKEY_ROOT = "user/ui/aas/controlDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

AasControlDialog::AasControlDialog() :
	TransientWindow(_("AAS Viewer"), GlobalMainFrame().getWxTopLevelWindow(), true),
	_dialogPanel(nullptr),
	_controlContainer(nullptr)
{
	populateWindow();

	InitialiseWindowPosition(135, 100, RKEY_WINDOW_STATE);
    SetMinClientSize(wxSize(135, 100));

	_mapEventSlot = GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &AasControlDialog::onMapEvent));
}

void AasControlDialog::onMapEvent(IMap::MapEvent ev)
{
	switch (ev)
	{
	case IMap::MapEvent::MapLoaded:
		refresh();
		break;
	case IMap::MapEvent::MapUnloading:
		clearControls();
		break;
	default:
		break;
	};
}

void AasControlDialog::populateWindow()
{
	wxScrolledWindow* dialogPanel = new wxScrolledWindow(this, wxID_ANY);
	dialogPanel->SetScrollRate(0, 15);

	_dialogPanel = dialogPanel;
	
	_dialogPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	_controlContainer = new wxFlexGridSizer(1, 2, 3, 3);
	_controlContainer->AddGrowableCol(0);

    _dialogPanel->GetSizer()->Add(_controlContainer, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 12);

	createButtons();

	_dialogPanel->FitInside(); // ask the sizer about the needed size
}

void AasControlDialog::createButtons()
{
	// Rescan button
	_rescanButton = new wxButton(_dialogPanel, wxID_ANY, _("Search for AAS Files"));
    _rescanButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev) { refresh(); });

	// Bind the toggle button to the registry key
	wxToggleButton* showNumbersButton = new wxToggleButton(_dialogPanel, wxID_ANY, _("Show Area Numbers"));
	registry::bindWidget(showNumbersButton, map::RKEY_SHOW_AAS_AREA_NUMBERS);
	
	wxToggleButton* hideDistantAreasButton = new wxToggleButton(_dialogPanel, wxID_ANY, _("Hide distant Areas"));
	registry::bindWidget(hideDistantAreasButton, map::RKEY_HIDE_DISTANT_AAS_AREAS);

	_dialogPanel->GetSizer()->Add(showNumbersButton, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);
	_dialogPanel->GetSizer()->Add(hideDistantAreasButton, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
	_dialogPanel->GetSizer()->Add(_rescanButton, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
}

void AasControlDialog::clearControls()
{
	// Remove all previously allocated controls
	_aasControls.clear();

	// Delete all wxWidgets objects 
	_controlContainer->Clear(true);
}

void AasControlDialog::refresh()
{
	clearControls();

	std::map<std::string, AasControlPtr> sortedControls;

	// Find all available AAS files for the current map
    
    std::list<map::AasFileInfo> aasFiles = GlobalAasFileManager().getAasFilesForMap(GlobalMap().getMapName());

    for (map::AasFileInfo& info : aasFiles)
    {
        // Create a new control for each AAS type
        // Store the object in a sorted container
        sortedControls[info.type.fileExtension] = std::make_shared<AasControl>(_dialogPanel, info);
    }

    // Assign all controls to the target vector, alphabetically sorted
    for (const auto& pair : sortedControls)
    {
        _aasControls.push_back(pair.second);
    }

	_controlContainer->SetRows(static_cast<int>(_aasControls.size()));

	for (AasControls::iterator i = _aasControls.begin(); i != _aasControls.end(); ++i)
	{
		_controlContainer->Add((*i)->getToggle(), 1, wxEXPAND);
		_controlContainer->Add((*i)->getButtons(), 0, wxEXPAND);

        if (i == _aasControls.begin())
        {
            // Prevent setting the focus on the buttons at the bottom which lets the scrollbar 
            // of the window jump around, set the focus on the first button.
            (*i)->getToggle()->SetFocus();
        }
	}

	_controlContainer->Layout();
	_dialogPanel->FitInside(); // ask the sizer about the needed size

	update();
}

void AasControlDialog::update()
{
	// Broadcast the update() call
	for (const AasControlPtr& control : _aasControls)
	{
		control->update();
	}
}

// TransientWindow callbacks
void AasControlDialog::_preShow()
{
	TransientWindow::_preShow();

	// Re-populate the dialog
	refresh();
}

void AasControlDialog::OnRadiantStartup()
{
    // Lookup the stored window information in the registry
    if (GlobalRegistry().getAttribute(RKEY_WINDOW_STATE, "visible") == "1")
    {
        // Show dialog
        Instance().Show();
    }
}

void AasControlDialog::onRadiantShutdown()
{
	rMessage() << "AasControlDialog shutting down." << std::endl;

	_mapEventSlot.disconnect();

	// Write the visibility status to the registry
	GlobalRegistry().setAttribute(RKEY_WINDOW_STATE, "visible", IsShownOnScreen() ? "1" : "0");

    // Hide the window and save its state
    if (IsShownOnScreen())
    {
        Hide();
    }

	// Destroy the window (after it has been disconnected from the Eventmanager)
	SendDestroyEvent();

	// Final step: clear the instance pointer
	InstancePtr().reset();
}

AasControlDialogPtr& AasControlDialog::InstancePtr()
{
	static AasControlDialogPtr _instancePtr;
	return _instancePtr;
}

AasControlDialog& AasControlDialog::Instance()
{
	AasControlDialogPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new AasControlDialog);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &AasControlDialog::onRadiantShutdown)
        );
	}

	return *instancePtr;
}

void AasControlDialog::Init()
{
    GlobalCommandSystem().addCommand("ToggleAasControlDialog", AasControlDialog::Toggle);
	GlobalEventManager().addCommand("ToggleAasControlDialog", "ToggleAasControlDialog");

    GlobalRadiant().signal_radiantStarted().connect(
        sigc::ptr_fun(&AasControlDialog::OnRadiantStartup)
    );
}

void AasControlDialog::Toggle(const cmd::ArgumentList& args)
{
	Instance().ToggleVisibility();
}

}
