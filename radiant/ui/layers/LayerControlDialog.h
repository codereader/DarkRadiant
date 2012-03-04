#ifndef LAYER_CONTROL_DIALOG_H_
#define LAYER_CONTROL_DIALOG_H_

#include "iradiant.h"
#include "icommandsystem.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/WindowPosition.h"
#include "LayerControl.h"
#include <boost/shared_ptr.hpp>

namespace Gtk
{
	class Table;
	class Button;
}

namespace ui
{

class LayerControlDialog;
typedef boost::shared_ptr<LayerControlDialog> LayerControlDialogPtr;

class LayerControlDialog :
	public gtkutil::PersistentTransientWindow
{
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	typedef std::vector<LayerControlPtr> LayerControls;
	LayerControls _layerControls;

	Gtk::Table* _controlContainer;

	Gtk::Button* _showAllLayers;
	Gtk::Button* _hideAllLayers;

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
	void _preHide();

	void populateWindow();

	// Creates the option buttons
	Gtk::Widget& createButtons();

	void onShowAllLayers();
	void onHideAllLayers();
};

} // namespace ui

#endif /* LAYER_CONTROL_DIALOG_H_ */
