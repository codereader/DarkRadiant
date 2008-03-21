#ifndef LAYER_CONTROL_DIALOG_H_
#define LAYER_CONTROL_DIALOG_H_

#include "gtkutil/window/PersistentTransientWindow.h"

namespace ui { 

class LayerControlDialog :
	public gtkutil::PersistentTransientWindow
{
public:
	LayerControlDialog();

	// Re-populates the window
	void update();

	// Toggles the visibility of the dialog
	void toggleDialog();

	// Command target (registered in the event manager)
	static void toggle();

	static LayerControlDialog& Instance();
};

} // namespace ui

#endif /* LAYER_CONTROL_DIALOG_H_ */
