#ifndef LAYERCONTROL_H_
#define LAYERCONTROL_H_

#include <boost/shared_ptr.hpp>

#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/togglebutton.h>

namespace ui
{

/**
 * greebo: A LayerControl contains a set of widgets needed
 *         to control an associated layer.
 *
 * Multiple LayerControls can be packed as children into the
 * owning LayerControlDialog.
 */
class LayerControl
{
private:
	// The ID of the associated layer
	int _layerID;

	Gtk::ToggleButton* _toggle;
	Gtk::Button* _labelButton;
	Gtk::Button* _deleteButton;
	Gtk::Button* _renameButton;
	Gtk::HBox* _buttonHBox;

	// Locks down the callbacks during widget update
	bool _updateActive;

public:
	LayerControl(int layerID);

	// Returns the widgets for packing this object into a container/table
	Gtk::Button& getLabelButton();
	Gtk::HBox& getButtons();
	Gtk::ToggleButton& getToggle();

	// Updates the state of all widgets
	void update();

private:
	void onToggle();
	void onDelete();
	void onRename();
	void onLayerSelect();
};
typedef boost::shared_ptr<LayerControl> LayerControlPtr;

} // namespace ui

#endif /* LAYERCONTROL_H_ */
