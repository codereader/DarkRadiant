#pragma once

#include "AasFileControl.h"
#include "imap.h"
#include <sigc++/connection.h>

#include "wxutil/DockablePanel.h"

class wxFlexGridSizer;
class wxButton;

namespace ui
{

class AasVisualisationPanel :
	public wxutil::DockablePanel
{
private:
    std::vector<AasFileControlPtr> _aasControls;

	wxPanel* _dialogPanel;

	wxFlexGridSizer* _controlContainer;
    wxButton* _rescanButton;

	sigc::connection _mapEventSlot;

public:
	AasVisualisationPanel(wxWindow* parent);
    ~AasVisualisationPanel() override;

protected:
    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectListeners();
    void disconnectListeners();

    // Re-populates the window
	void refresh();

	// Updates the state of all controls
	void update();

	void populateWindow();
    void createButtons();
	void clearControls();

	void onMapEvent(IMap::MapEvent ev);
};

}
