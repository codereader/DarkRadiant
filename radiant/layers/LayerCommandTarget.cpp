#include "LayerCommandTarget.h"

#include "ieventmanager.h"
#include "LayerSystem.h"
#include "string/string.h"

namespace scene {

LayerCommandTarget::LayerCommandTarget(int layerID) :
	_layerID(layerID)
{
	GlobalEventManager().addCommand(
		COMMAND_PREFIX_ADDTOLAYER + intToStr(_layerID), 
		AddSelectionCaller(*this)
	);

	GlobalEventManager().addCommand(
		COMMAND_PREFIX_MOVETOLAYER + intToStr(_layerID), 
		MoveSelectionCaller(*this)
	);

	GlobalEventManager().addCommand(
		COMMAND_PREFIX_SHOWLAYER + intToStr(_layerID), 
		ShowLayerCaller(*this)
	);

	GlobalEventManager().addCommand(
		COMMAND_PREFIX_HIDELAYER + intToStr(_layerID), 
		HideLayerCaller(*this)
	);
}

void LayerCommandTarget::addSelectionToLayer() {
	// Pass the call to the LayerSystem
	getLayerSystem().addSelectionToLayer(_layerID);
}

void LayerCommandTarget::moveSelectionToLayer() {
	// Pass the call to the LayerSystem
	getLayerSystem().moveSelectionToLayer(_layerID);
}

void LayerCommandTarget::showLayer() {
	getLayerSystem().setLayerVisibility(_layerID, true);
}

void LayerCommandTarget::hideLayer() {
	getLayerSystem().setLayerVisibility(_layerID, false);
}

} // namespace scene
