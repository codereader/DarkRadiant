#ifndef LAYERCONTEXTMENU_H_
#define LAYERCONTEXTMENU_H_

#include <boost/function.hpp>

#include <gtkmm/menu.h>
#include "layers/LayerSystem.h"

namespace ui {

class LayerContextMenu :
	public scene::LayerSystem::Visitor,
	public Gtk::Menu
{
public:
	// The function to be called on menu selection, the ID of the
	// selected layer is passed along.
	typedef boost::function<void(int layer)> OnSelectionFunc;

private:
	OnSelectionFunc _onSelection;

	typedef std::map<std::string, int> SortedLayerMap;
	SortedLayerMap _sortedLayers;

public:
	LayerContextMenu(OnSelectionFunc& onSelection);

	// scene::LayerSystem::Visitor implementation
	void visit(int layerID, const std::string& layerName);

private:
	// Creates the menu items
	void createMenuItems();

	// gtkmm Callback for menu selections, layerId is bound on connection
	void onActivate(int layerId);
};
typedef boost::shared_ptr<LayerContextMenu> LayerContextMenuPtr;

} // namespace ui

#endif /* LAYERCONTEXTMENU_H_ */
