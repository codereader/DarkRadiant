#include "LayerCommandTarget.h"

#include "ieventmanager.h"
#include "LayerSystem.h"

namespace scene {

LayerCommandTarget::LayerCommandTarget(int layerID) :
	_layerID(layerID)
{
	GlobalEventManager().addCommand("", AddSelectionCaller(*this));
}

// Command target, this adds the current selection to the associated layer
void LayerCommandTarget::addSelectionToLayer() {
	// Pass the call to the LayerSystem
	getLayerSystem().addSelectionToLayer(_layerID);
}

} // namespace scene
