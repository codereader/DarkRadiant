#include "AasControlDialog.h"

#include "iradiant.h"

namespace ui
{

namespace
{
	const std::string RKEY_ROOT = "user/ui/aas/controlDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

AasControlDialog::AasControlDialog() :
	TransientWindow(_("AAS Visualisation"), GlobalMainFrame().getWxTopLevelWindow(), true),
	_dialogPanel(nullptr),
	_controlContainer(nullptr)
{
	populateWindow();

	InitialiseWindowPosition(230, 400, RKEY_WINDOW_STATE);
    SetMinClientSize(wxSize(230, 200));
}

void AasControlDialog::populateWindow()
{
	wxScrolledWindow* dialogPanel = new wxScrolledWindow(this, wxID_ANY);
	dialogPanel->SetScrollRate(0, 15);

	_dialogPanel = dialogPanel;
	
	_dialogPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	_controlContainer = new wxFlexGridSizer(1, 3, 3, 3);
	_controlContainer->AddGrowableCol(1);

    _dialogPanel->GetSizer()->Add(_controlContainer, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 12);

	_dialogPanel->FitInside(); // ask the sizer about the needed size
}

void AasControlDialog::refresh()
{
	// Delete all wxWidgets objects first
	_controlContainer->Clear(true);
#if 0
	// Remove all previously allocated layercontrols
	_layerControls.clear();

    std::map<std::string, LayerControlPtr> sortedControls;

	// Traverse the layers
    scene::getLayerSystem().foreachLayer([&](int layerID, const std::string& layerName)
    {
        // Create a new layercontrol for each visited layer
        // Store the object in a sorted container
        sortedControls[layerName] = LayerControlPtr(new LayerControl(_dialogPanel, layerID));
    });

    // Assign all controls to the target vector, alphabetically sorted
    for (std::pair<std::string, LayerControlPtr> pair : sortedControls)
    {
        _layerControls.push_back(pair.second);
    }

	_controlContainer->SetRows(static_cast<int>(_layerControls.size()));

	int c = 0;
	for (LayerControls::iterator i = _layerControls.begin();
		 i != _layerControls.end(); ++i, ++c)
	{
		_controlContainer->Add((*i)->getToggle(), 0);
		_controlContainer->Add((*i)->getLabelButton(), 0, wxEXPAND);
		_controlContainer->Add((*i)->getButtons(), 0, wxEXPAND);

        if (c == 0)
        {
            // Prevent setting the focus on the buttons at the bottom which lets the scrollbar 
            // of the window jump around (#4089), set the focus on the first button.
            (*i)->getLabelButton()->SetFocus();
        }
	}

	_controlContainer->Layout();
	_dialogPanel->FitInside(); // ask the sizer about the needed size

	update();
#endif
}

void AasControlDialog::update()
{
#if 0
	// Broadcast the update() call
	for (LayerControls::iterator i = _layerControls.begin();
		 i != _layerControls.end(); ++i)
	{
		(*i)->update();
	}

	// Update the show/hide all button sensitiveness
    std::size_t numVisible;
    std::size_t numHidden;

    GlobalLayerSystem().foreachLayer([&](int layerID, const std::string& layerName)
    {
        if (GlobalLayerSystem().layerIsVisible(layerID))
        {
            numVisible++;
        }
        else
        {
            numHidden++;
        }
    });
#endif
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
