#include "LayerOrthoContextMenuItem.h"

#include "ilayer.h"
#include "iuimanager.h"
#include "selectionlib.h"
#include "map/Map.h"

namespace ui
{

namespace
{
	const char* const LAYER_ICON = "layers.png";
}

LayerOrthoContextMenuItem::LayerOrthoContextMenuItem(const std::string& caption,
											 LayerContextMenu::OnSelectionFunc callback) :
	wxutil::IconTextMenuItem(caption, LAYER_ICON),
	_func(callback)
{
	// Re-populate the submenu
	_submenu = new LayerContextMenu(_func);

	// wxWidgets will take care of deleting the submenu
	SetSubMenu(_submenu);
}

LayerOrthoContextMenuItem::~LayerOrthoContextMenuItem()
{
	if (GetMenu() != nullptr)
	{
		// Destroying a menu item doesn't de-register it from the parent menu
		// To prevent double-deletions, we de-register the item on our own
		GetMenu()->Remove(GetId());
	}
}

wxMenuItem* LayerOrthoContextMenuItem::getMenuItem()
{
	return this;
}

void LayerOrthoContextMenuItem::execute()
{}

bool LayerOrthoContextMenuItem::isSensitive()
{
	return GlobalSelectionSystem().getSelectionInfo().totalCount > 0;
}

void LayerOrthoContextMenuItem::preShow()
{
	// Re-populate the submenu
	_submenu->populate();
}

void LayerOrthoContextMenuItem::AddToLayer(int layerID)
{
	DoWithMapLayerManager([=](scene::ILayerManager& manager)
	{
		manager.addSelectionToLayer(layerID);
		GlobalMap().setModified(true);
	});
}

void LayerOrthoContextMenuItem::MoveToLayer(int layerID)
{
	DoWithMapLayerManager([=](scene::ILayerManager& manager)
	{
		manager.moveSelectionToLayer(layerID);
		GlobalMap().setModified(true);
	});
}

void LayerOrthoContextMenuItem::RemoveFromLayer(int layerID)
{
	DoWithMapLayerManager([=](scene::ILayerManager& manager)
	{
		manager.removeSelectionFromLayer(layerID);
		GlobalMap().setModified(true);
	});
}

void LayerOrthoContextMenuItem::DoWithMapLayerManager(const std::function<void(scene::ILayerManager&)>& func)
{
	if (!GlobalMapModule().getRoot())
	{
		return;
	}

	func(GlobalMapModule().getRoot()->getLayerManager());
}

} // namespace
