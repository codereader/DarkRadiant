#pragma once

#include "imap.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "wxutil/window/TransientWindow.h"
#include "LayerControl.h"

#include <wx/panel.h>
#include <memory>
#include <sigc++/connection.h>

class wxButton;
class wxFlexGridSizer;
class wxWindow;
class wxCommandEvent;

namespace ui
{

class LayerControlDialog;
typedef std::shared_ptr<LayerControlDialog> LayerControlDialogPtr;

class LayerControlDialog :
	public wxutil::TransientWindow
{
private:
	typedef std::vector<LayerControlPtr> LayerControls;
	LayerControls _layerControls;

	wxPanel* _dialogPanel;

	wxFlexGridSizer* _controlContainer;

	wxButton* _showAllLayers;
	wxButton* _hideAllLayers;

	bool _rescanSelectionOnIdle;
	sigc::connection _selectionChangedSignal;
	sigc::connection _layersChangedSignal;
	sigc::connection _layerVisibilityChangedSignal;
	sigc::connection _nodeLayerMembershipChangedSignal;
	sigc::connection _mapEventSignal;

public:
	LayerControlDialog();

	void onRadiantShutdown();

	// Re-populates the window
	void refresh();

	// Updates the state of all LayerControls
	void update();

	// Command target (registered in the event manager)
	static void toggle(const cmd::ArgumentList& args);

	// Checks if dialog should be shown after startup
	static void onRadiantStartup();

	static LayerControlDialog& Instance();

private:
	static LayerControlDialogPtr& InstancePtr();

	// TransientWindow events
	void _preShow() override;
	void _postHide() override;

	void populateWindow();

	// Update the usage colours on the controls
	void updateLayerUsage();

	// Creates the option buttons
	void createButtons();

	void onShowAllLayers(wxCommandEvent& ev);
	void onHideAllLayers(wxCommandEvent& ev);
	void onIdle();

	void onMapEvent(IMap::MapEvent ev);
};

} // namespace ui
