#include "AasVisualisationPanel.h"

#include "i18n.h"
#include "iaasfile.h"
#include "imap.h"

#include <wx/button.h>
#include <wx/tglbtn.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>

#include "registry/Widgets.h"

namespace ui
{

AasVisualisationPanel::AasVisualisationPanel(wxWindow* parent) :
    DockablePanel(parent),
	_dialogPanel(nullptr),
	_controlContainer(nullptr)
{
	populateWindow();

    SetMinClientSize(wxSize(135, 100));
}

AasVisualisationPanel::~AasVisualisationPanel()
{
    if (panelIsActive())
    {
        disconnectListeners();
    }
}

void AasVisualisationPanel::onPanelActivated()
{
    connectListeners();
    refresh();
}

void AasVisualisationPanel::onPanelDeactivated()
{
    disconnectListeners();
}

void AasVisualisationPanel::connectListeners()
{
    _mapEventSlot = GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(*this, &AasVisualisationPanel::onMapEvent));
}

void AasVisualisationPanel::disconnectListeners()
{
    _mapEventSlot.disconnect();
}

void AasVisualisationPanel::onMapEvent(IMap::MapEvent ev)
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

void AasVisualisationPanel::populateWindow()
{
    auto scrollView = new wxScrolledWindow(this, wxID_ANY);
    scrollView->SetScrollRate(0, 15);

    _dialogPanel = scrollView;

    _dialogPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

    _controlContainer = new wxFlexGridSizer(1, 2, 3, 3);
    _controlContainer->AddGrowableCol(0);

    _dialogPanel->GetSizer()->Add(_controlContainer, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 12);

    createButtons();

    _dialogPanel->FitInside(); // ask the sizer about the needed size

    SetSizer(new wxBoxSizer(wxVERTICAL));
    GetSizer()->Add(scrollView, 1, wxEXPAND);
}

void AasVisualisationPanel::createButtons()
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

void AasVisualisationPanel::clearControls()
{
	// Remove all previously allocated controls
	_aasControls.clear();

	// Delete all wxWidgets objects 
	_controlContainer->Clear(true);
}

void AasVisualisationPanel::refresh()
{
	clearControls();

	std::map<std::string, AasFileControlPtr> sortedControls;

	// Find all available AAS files for the current map
    
    std::list<map::AasFileInfo> aasFiles = GlobalAasFileManager().getAasFilesForMap(GlobalMapModule().getMapName());

    for (map::AasFileInfo& info : aasFiles)
    {
        // Create a new control for each AAS type
        // Store the object in a sorted container
        sortedControls[info.type.fileExtension] = std::make_shared<AasFileControl>(_dialogPanel, info);
    }

    // Assign all controls to the target vector, alphabetically sorted
    for (const auto& pair : sortedControls)
    {
        _aasControls.push_back(pair.second);
    }

	_controlContainer->SetRows(static_cast<int>(_aasControls.size()));

	for (auto i = _aasControls.begin(); i != _aasControls.end(); ++i)
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

void AasVisualisationPanel::update()
{
	// Broadcast the update() call
	for (const auto& control : _aasControls)
	{
		control->update();
	}
}

}
