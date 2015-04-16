#pragma once

#include <functional>
#include <map>
#include <memory>

#include <wx/menu.h>

namespace ui 
{

class LayerContextMenu :
	public wxMenu
{
public:
	// The function to be called on menu selection, the ID of the
	// selected layer is passed along.
	typedef std::function<void(int layer)> OnSelectionFunc;

private:
	OnSelectionFunc _onSelection;

	typedef std::map<std::string, int> SortedLayerMap;
	SortedLayerMap _sortedLayers;

	typedef std::map<int, int> MenuItemIdToLayerMapping;
	MenuItemIdToLayerMapping _menuItemMapping;

public:
	LayerContextMenu(OnSelectionFunc& onSelection);

	// scene::LayerSystem::Visitor implementation
	void visit(int layerID, const std::string& layerName);

	// Loads layer names into the menu, clears existing items first
	void populate();

private:
	// Creates the menu items
	void createMenuItems();

	// wx Callback for menu selections
	void onActivate(wxCommandEvent& ev);
};
typedef std::shared_ptr<LayerContextMenu> LayerContextMenuPtr;

} // namespace ui
