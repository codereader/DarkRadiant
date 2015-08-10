#pragma once

#include "wxutil/window/TransientWindow.h"

namespace ui
{

class AasControlDialog;
typedef std::shared_ptr<AasControlDialog> AasControlDialogPtr;

class AasControlDialog :
	public wxutil::TransientWindow
{
private:
	//typedef std::vector<AasControlPtr> LayerControls;
	//LayerControls _layerControls;

	wxPanel* _dialogPanel;

	wxFlexGridSizer* _controlContainer;

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
    void onRadiantShutdown();
};

}
