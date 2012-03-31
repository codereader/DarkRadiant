#include "LayerCommandTarget.h"

#include "ieventmanager.h"
#include "icommandsystem.h"
#include "LayerSystem.h"
#include "string/string.h"
#include <boost/bind.hpp>

namespace scene {

LayerCommandTarget::LayerCommandTarget(int layerID) :
	_layerID(layerID)
{
	GlobalCommandSystem().addCommand(
		COMMAND_PREFIX_ADDTOLAYER + string::to_string(_layerID),
		boost::bind(&LayerCommandTarget::addSelectionToLayer, this, _1)
	);
	GlobalEventManager().addCommand(
		COMMAND_PREFIX_ADDTOLAYER + string::to_string(_layerID),
		COMMAND_PREFIX_ADDTOLAYER + string::to_string(_layerID)
	);

	GlobalCommandSystem().addCommand(
		COMMAND_PREFIX_MOVETOLAYER + string::to_string(_layerID),
		boost::bind(&LayerCommandTarget::moveSelectionToLayer, this, _1)
	);
	GlobalEventManager().addCommand(
		COMMAND_PREFIX_MOVETOLAYER + string::to_string(_layerID),
		COMMAND_PREFIX_MOVETOLAYER + string::to_string(_layerID)
	);

	GlobalCommandSystem().addCommand(
		COMMAND_PREFIX_SHOWLAYER + string::to_string(_layerID),
		boost::bind(&LayerCommandTarget::showLayer, this, _1)
	);
	GlobalEventManager().addCommand(
		COMMAND_PREFIX_SHOWLAYER + string::to_string(_layerID),
		COMMAND_PREFIX_SHOWLAYER + string::to_string(_layerID)
	);

	GlobalCommandSystem().addCommand(
		COMMAND_PREFIX_HIDELAYER + string::to_string(_layerID),
		boost::bind(&LayerCommandTarget::hideLayer, this, _1)
	);
	GlobalEventManager().addCommand(
		COMMAND_PREFIX_HIDELAYER + string::to_string(_layerID),
		COMMAND_PREFIX_HIDELAYER + string::to_string(_layerID)
	);
}

void LayerCommandTarget::addSelectionToLayer(const cmd::ArgumentList& args) {
	// Pass the call to the LayerSystem
	getLayerSystem().addSelectionToLayer(_layerID);
}

void LayerCommandTarget::moveSelectionToLayer(const cmd::ArgumentList& args) {
	// Pass the call to the LayerSystem
	getLayerSystem().moveSelectionToLayer(_layerID);
}

void LayerCommandTarget::showLayer(const cmd::ArgumentList& args) {
	getLayerSystem().setLayerVisibility(_layerID, true);
}

void LayerCommandTarget::hideLayer(const cmd::ArgumentList& args) {
	getLayerSystem().setLayerVisibility(_layerID, false);
}

} // namespace scene
