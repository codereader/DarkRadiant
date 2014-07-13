#pragma once

#include "iradiant.h"
#include "icommandsystem.h"
#include "wxutil/window/TransientWindow.h"
#include "LayerControl.h"

#include <wx/panel.h>
#include <boost/shared_ptr.hpp>

class wxButton;
class wxFlexGridSizer;
class wxWindow;
class wxCommandEvent;

namespace ui
{

class LayerControlDialog;
typedef boost::shared_ptr<LayerControlDialog> LayerControlDialogPtr;

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

public:
	LayerControlDialog();

	void onRadiantShutdown();

	// Re-populates the window
	void refresh();

	// Updates the state of all LayerControls
	void update();

	// Command target (registered in the event manager)
	static void toggle(const cmd::ArgumentList& args);

	// Called during mainframe construction
	static void init();

	static LayerControlDialog& Instance();

private:
	static LayerControlDialogPtr& InstancePtr();

	// TransientWindow events
	void _preShow();

	void populateWindow();

	// Creates the option buttons
	void createButtons();

	void onShowAllLayers(wxCommandEvent& ev);
	void onHideAllLayers(wxCommandEvent& ev);
};

} // namespace ui
