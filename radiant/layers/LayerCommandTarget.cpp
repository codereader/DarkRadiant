#include "LayerCommandTarget.h"

#include "ieventmanager.h"
#include "LayerSystem.h"
#include "string/string.h"

namespace scene {

LayerCommandTarget::LayerCommandTarget(int layerID) :
	_layerID(layerID)
{
	GlobalEventManager().addCommand(
		"AddSelectionToLayer" + intToStr(_layerID), 
		AddSelectionCaller(*this)
	);

	GlobalEventManager().addCommand(
		"MoveSelectionToLayer" + intToStr(_layerID), 
		MoveSelectionCaller(*this)
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

} // namespace scene
