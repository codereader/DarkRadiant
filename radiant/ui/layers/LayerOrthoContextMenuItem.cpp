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
	gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(LAYER_ICON), caption),
	_func(callback)
{}

Gtk::MenuItem* LayerOrthoContextMenuItem::getWidget()
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
	_submenu = LayerContextMenuPtr(new LayerContextMenu(_func));

	// Cast the LayerContextMenu onto GtkWidget* and pack it
	set_submenu(*_submenu);

	show_all();
}

void LayerOrthoContextMenuItem::AddToLayer(int layerID)
{
	GlobalLayerSystem().addSelectionToLayer(layerID);
	GlobalMap().setModified(true);
}

void LayerOrthoContextMenuItem::MoveToLayer(int layerID)
{
	GlobalLayerSystem().moveSelectionToLayer(layerID);
	GlobalMap().setModified(true);
}

void LayerOrthoContextMenuItem::RemoveFromLayer(int layerID)
{
	GlobalLayerSystem().removeSelectionFromLayer(layerID);
	GlobalMap().setModified(true);
}

} // namespace
