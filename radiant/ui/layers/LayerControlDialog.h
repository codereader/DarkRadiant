#ifndef LAYER_CONTROL_DIALOG_H_
#define LAYER_CONTROL_DIALOG_H_

#include "iradiant.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/WindowPosition.h"
#include "LayerControl.h"
#include <boost/shared_ptr.hpp>

namespace ui { 

class LayerControlDialog;
typedef boost::shared_ptr<LayerControlDialog> LayerControlDialogPtr;

class LayerControlDialog :
	public gtkutil::PersistentTransientWindow,
	public RadiantEventListener
{
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	typedef std::vector<LayerControlPtr> LayerControls;
	LayerControls _layerControls;

	GtkWidget* _controlContainer;

	GtkWidget* _showAllLayers;
	GtkWidget* _hideAllLayers;

public:
	LayerControlDialog();

	// RadiantEventListener implementation
	void onRadiantShutdown();

	// Re-populates the window
	void refresh();

	// Updates the state of all LayerControls
	void update();

	// Toggles the visibility of the dialog
	void toggleDialog();

	// Command target (registered in the event manager)
	static void toggle();

	// Called during mainframe construction
	static void init();

	static LayerControlDialog& Instance();
	static LayerControlDialogPtr& InstancePtr();

private:
	// TransientWindow events
	virtual void _preShow();
	virtual void _preHide();

	void populateWindow();

	// Creates the option buttons
	GtkWidget* createButtons();

	static void onCreateLayer(GtkWidget* button, LayerControlDialog* self);
	static void onShowAllLayers(GtkWidget* button, LayerControlDialog* self);
	static void onHideAllLayers(GtkWidget* button, LayerControlDialog* self);
};

} // namespace ui

#endif /* LAYER_CONTROL_DIALOG_H_ */
