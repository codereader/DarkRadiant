#include "LayerControlDialog.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "ilayer.h"
#include "imainframe.h"

#include "registry/registry.h"

#include <wx/button.h>
#include <wx/bmpbuttn.h>
#include <wx/tglbtn.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/artprov.h>
#include <wx/scrolwin.h>

#include "layers/LayerSystem.h"

namespace ui
{

namespace
{
	const std::string RKEY_ROOT = "user/ui/layers/controlDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

LayerControlDialog::LayerControlDialog() :
	TransientWindow(_("Layers"), GlobalMainFrame().getWxTopLevelWindow(), true),
	_dialogPanel(NULL),
	_controlContainer(NULL),
	_showAllLayers(NULL),
	_hideAllLayers(NULL)
{
	populateWindow();

	InitialiseWindowPosition(230, 400, RKEY_WINDOW_STATE);
    SetMinClientSize(wxSize(230, 200));
}

void LayerControlDialog::populateWindow()
{
	wxScrolledWindow* dialogPanel = new wxScrolledWindow(this, wxID_ANY);
	dialogPanel->SetScrollRate(0, 15);

	_dialogPanel = dialogPanel;
	
	_dialogPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	_controlContainer = new wxFlexGridSizer(1, 3, 3, 3);
	_controlContainer->AddGrowableCol(1);

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

	_showAllLayers->Connect(wxEVT_BUTTON, wxCommandEventHandler(LayerControlDialog::onShowAllLayers), NULL, this);
	_hideAllLayers->Connect(wxEVT_BUTTON, wxCommandEventHandler(LayerControlDialog::onHideAllLayers), NULL, this);

	// Create layer button
	wxButton* createButton = new wxButton(_dialogPanel, wxID_ANY, _("New"));
	createButton->SetBitmap(wxArtProvider::GetBitmap(wxART_PLUS));

	IEventPtr event = GlobalEventManager().findEvent("CreateNewLayer");

	if (event != NULL)
	{
		event->connectButton(createButton);
	}

	hideShowBox->Add(_showAllLayers, 1, wxEXPAND | wxTOP, 6);
	hideShowBox->Add(_hideAllLayers, 1, wxEXPAND | wxLEFT | wxTOP, 6);

    _dialogPanel->GetSizer()->Add(createButton, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 12);
    _dialogPanel->GetSizer()->Add(hideShowBox, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 12);
}

void LayerControlDialog::refresh()
{
	// Delete all wxWidgets objects first
	_controlContainer->Clear(true);

	// Remove all previously allocated layercontrols
	_layerControls.clear();

	// Local helper class for populating the window
	class LayerControlAccumulator :
		public scene::LayerSystem::Visitor
	{
		typedef std::map<std::string, LayerControlPtr> LayerControlMap;
		LayerControlMap _sortedLayerControls;

		wxPanel* _dialogPanel;
	public:
		LayerControlAccumulator(wxPanel* dialogPanel) :
			_dialogPanel(dialogPanel)
		{}

		void visit(int layerID, const std::string& layerName)
		{
			// Create a new layercontrol for each visited layer
			// Store the object in a sorted container
			_sortedLayerControls[layerName] = LayerControlPtr(new LayerControl(_dialogPanel, layerID));
		}

		// Returns the sorted vector
		LayerControls getVector()
		{
			LayerControls returnValue;

			// Copy the objects over to a linear vector
			for (LayerControlMap::const_iterator i = _sortedLayerControls.begin();
				 i != _sortedLayerControls.end(); ++i)
			{
				returnValue.push_back(i->second);
			}

			return returnValue;
		}
	} populator(_dialogPanel);

	// Traverse the layers
	scene::getLayerSystem().foreachLayer(populator);

	// Get the sorted vector
	_layerControls = populator.getVector();

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
}

void LayerControlDialog::update()
{
	// Broadcast the update() call
	for (LayerControls::iterator i = _layerControls.begin();
		 i != _layerControls.end(); ++i)
	{
		(*i)->update();
	}

	// Update the show/hide all button sensitiveness

	class CheckAllLayersWalker :
		public scene::ILayerSystem::Visitor
	{
	public:
		std::size_t numVisible;
		std::size_t numHidden;

		CheckAllLayersWalker() :
			numVisible(0),
			numHidden(0)
		{}

		void visit(int layerID, const std::string& layerName)
		{
			if (GlobalLayerSystem().layerIsVisible(layerID)) {
				numVisible++;
			}
			else {
				numHidden++;
			}
		}
	};

	CheckAllLayersWalker visitor;
	GlobalLayerSystem().foreachLayer(visitor);

	_showAllLayers->Enable(visitor.numHidden > 0);
	_hideAllLayers->Enable(visitor.numVisible > 0);
}

void LayerControlDialog::toggle(const cmd::ArgumentList& args)
{
	Instance().ToggleVisibility();
}

void LayerControlDialog::init()
{
	// Lookup the stored window information in the registry
	if (GlobalRegistry().getAttribute(RKEY_WINDOW_STATE, "visible") == "1")
	{
		// Show dialog
		Instance().Show();
	}
}

void LayerControlDialog::onRadiantShutdown()
{
	rMessage() << "LayerControlDialog shutting down." << std::endl;

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

LayerControlDialogPtr& LayerControlDialog::InstancePtr()
{
	static LayerControlDialogPtr _instancePtr;
	return _instancePtr;
}

LayerControlDialog& LayerControlDialog::Instance()
{
	LayerControlDialogPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new LayerControlDialog);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &LayerControlDialog::onRadiantShutdown)
        );
	}

	return *instancePtr;
}

// TransientWindow callbacks
void LayerControlDialog::_preShow()
{
	TransientWindow::_preShow();

	// Re-populate the dialog
	refresh();
}

void LayerControlDialog::onShowAllLayers(wxCommandEvent& ev)
{
	// Local helper class
	class ShowAllLayersWalker :
		public scene::ILayerSystem::Visitor
	{
	public:
		void visit(int layerID, const std::string& layerName)
		{
			GlobalLayerSystem().setLayerVisibility(layerID, true);
		}
	};

	ShowAllLayersWalker walker;
	GlobalLayerSystem().foreachLayer(walker);
}

void LayerControlDialog::onHideAllLayers(wxCommandEvent& ev)
{
	// Local helper class
	class HideAllLayersWalker :
		public scene::ILayerSystem::Visitor
	{
	public:
		void visit(int layerID, const std::string& layerName)
		{
			GlobalLayerSystem().setLayerVisibility(layerID, false);
		}
	};

	HideAllLayersWalker walker;
	GlobalLayerSystem().foreachLayer(walker);
}

} // namespace ui
