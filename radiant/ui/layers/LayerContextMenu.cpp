#include "LayerContextMenu.h"

#include "iuimanager.h"
#include "gtkutil/menu/IconTextMenuItem.h"
#include "layers/LayerSystem.h"

namespace ui
{

	namespace
	{
		const std::string LAYER_ICON("layer.png");
	}

LayerContextMenu::LayerContextMenu(OnSelectionFunc& onSelection) :
	wxMenu(),
	_onSelection(onSelection)
{
	// Populate the map with all layer names and IDs
	scene::getLayerSystem().foreachLayer(*this);

	// Create the menu items
	createMenuItems();
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
		wxMenuItem* menuItem = new wxutil::IconTextMenuItem(i->first, LAYER_ICON);

		// Connect the "onclick" signal, bind the layer ID
		//menuItem->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &LayerContextMenu::onActivate), i->second));

		// Add it to the parent menu
		Append(menuItem);

		// remember the layer id for this item
		_menuItemMapping[menuItem->GetId()] = i->second;

		Connect(menuItem->GetId(), wxEVT_MENU, wxCommandEventHandler(LayerContextMenu::onActivate), NULL, this);
	}
}

void LayerContextMenu::onActivate(wxCommandEvent& ev)
{
	assert(_menuItemMapping.find(ev.GetId()) != _menuItemMapping.end());

	int layerId = _menuItemMapping[ev.GetId()];

	// Pass the call to the function
	_onSelection(layerId);
}

} // namespace ui
