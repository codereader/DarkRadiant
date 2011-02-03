#include "LayerContextMenu.h"

#include "iuimanager.h"
#include "gtkutil/IconTextMenuItem.h"
#include "layers/LayerSystem.h"

namespace ui
{

	namespace
	{
		const std::string LAYER_ICON("layer.png");
	}

LayerContextMenu::LayerContextMenu(OnSelectionFunc& onSelection) :
	Gtk::Menu(),
	_onSelection(onSelection)
{
	// Populate the map with all layer names and IDs
	scene::getLayerSystem().foreachLayer(*this);

	// Create the menu items
	createMenuItems();

	// Show all the items
	show_all();
}

void LayerContextMenu::visit(int layerID, const std::string& layerName)
{
	_sortedLayers.insert(
		SortedLayerMap::value_type(layerName, layerID)
	);
}

void LayerContextMenu::createMenuItems()
{
	for (SortedLayerMap::const_iterator i = _sortedLayers.begin();
		i != _sortedLayers.end(); ++i)
	{
		// Create a new menuitem
		Gtk::MenuItem* menuItem = Gtk::manage(new gtkutil::IconTextMenuItem(
			GlobalUIManager().getLocalPixbuf(LAYER_ICON), i->first));

		// Connect the "onclick" signal, bind the layer ID
		menuItem->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &LayerContextMenu::onActivate), i->second));

		// Add it to the parent menu
		append(*menuItem);
	}
}

void LayerContextMenu::onActivate(int layerId)
{
	// Pass the call to the function
	_onSelection(layerId);
}

} // namespace ui
