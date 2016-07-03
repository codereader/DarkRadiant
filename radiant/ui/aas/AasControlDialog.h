#pragma once

#include "wxutil/window/TransientWindow.h"
#include "AasControl.h"
#include "imap.h"
#include "icommandsystem.h"
#include <sigc++/connection.h>

class wxFlexGridSizer;
class wxButton;
class wxPanel;

namespace ui
{

class AasControlDialog;
typedef std::shared_ptr<AasControlDialog> AasControlDialogPtr;

class AasControlDialog :
	public wxutil::TransientWindow
{
private:
	typedef std::vector<AasControlPtr> AasControls;
	AasControls _aasControls;

	wxPanel* _dialogPanel;

	wxFlexGridSizer* _controlContainer;
    wxButton* _rescanButton;

	sigc::connection _mapEventSlot;

public:
	AasControlDialog();

    // Re-populates the window
	void refresh();

	// Updates the state of all controls
	void update();

	// Command target (registered in the event manager)
	static void Toggle(const cmd::ArgumentList& args);

	// Called during AAS module initialisation
	static void Init();
    static void OnRadiantStartup();

	static AasControlDialog& Instance();

private:
	static AasControlDialogPtr& InstancePtr();

	// TransientWindow events
	void _preShow();

	void populateWindow();
    void createButtons();
    void onRadiantShutdown();
	void clearControls();

	void onMapEvent(IMap::MapEvent ev);
};

}
