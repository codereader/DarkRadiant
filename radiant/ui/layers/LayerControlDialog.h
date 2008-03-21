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

public:
	LayerControlDialog();

	// RadiantEventListener implementation
	void onRadiantShutdown();

	// Re-populates the window
	void update();

	// Toggles the visibility of the dialog
	void toggleDialog();

	// Command target (registered in the event manager)
	static void toggle();

	static LayerControlDialog& Instance();
	static LayerControlDialogPtr& InstancePtr();
};

} // namespace ui

#endif /* LAYER_CONTROL_DIALOG_H_ */
