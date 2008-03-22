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

	GtkWidget* _toggle;
	GtkWidget* _label;
	GtkWidget* _deleteButton;

	// Locks down the callbacks during widget update
	bool _updateActive;

public:
	LayerControl(int layerID);

	// Returns the widgets for packing this object into a container/table
	GtkWidget* getLabel() const;
	GtkWidget* getButtons() const;
	GtkWidget* getToggle() const;

	// Updates the state of all widgets
	void update();

private:
	static void onToggle(GtkToggleButton* togglebutton, LayerControl* self);
	static void onDelete(GtkWidget* button, LayerControl* self);
};
typedef boost::shared_ptr<LayerControl> LayerControlPtr;

} // namespace ui

#endif /* LAYERCONTROL_H_ */
