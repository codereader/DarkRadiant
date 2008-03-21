#ifndef LAYERCONTROL_H_
#define LAYERCONTROL_H_

#include <boost/shared_ptr.hpp>

typedef struct _GtkWidget GtkWidget;

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

public:
	LayerControl(int layerID);

	// Operator cast for packing this object into a container
	operator GtkWidget*();
};
typedef boost::shared_ptr<LayerControl> LayerControlPtr;

} // namespace ui

#endif /* LAYERCONTROL_H_ */
