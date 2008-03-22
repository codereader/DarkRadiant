#ifndef LAYERCONTROL_H_
#define LAYERCONTROL_H_

#include <boost/shared_ptr.hpp>

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkToggleButton GtkToggleButton;

namespace ui {

/** 
 * greebo: A LayerControl contains a set of widgets needed
 *         to control an associated layer.
 *
 * Multiple LayerControls can be packed as children into the 
 * owning LayerControlDialog.
 */
class LayerControl
{
	// The ID of the associated layer
	int _layerID;

	// The hbox containing the control widgets
	GtkWidget* _hbox;

	GtkWidget* _toggle;
	GtkWidget* _label;

	// Locks down the callbacks during widget update
	bool _updateActive;

public:
	LayerControl(int layerID);

	// Returns the widget for packing this object into a container
	GtkWidget* getWidget() const;

	// Updates the state of all widgets
	void update();

private:
	static void onToggle(GtkToggleButton* togglebutton, LayerControl* self);
};
typedef boost::shared_ptr<LayerControl> LayerControlPtr;

} // namespace ui

#endif /* LAYERCONTROL_H_ */
