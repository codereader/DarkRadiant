#include "LayerCommandTarget.h"

#include "ieventmanager.h"
#include "imap.h"
#include "icommandsystem.h"
#include "LayerManager.h"
#include "string/string.h"
#include <functional>

namespace scene {

LayerCommandTarget::LayerCommandTarget(int layerID) :
	_layerID(layerID)
{
	GlobalCommandSystem().addCommand(
		COMMAND_PREFIX_ADDTOLAYER + string::to_string(_layerID),
		std::bind(&LayerCommandTarget::addSelectionToLayer, this, std::placeholders::_1)
	);
	GlobalEventManager().addCommand(
		COMMAND_PREFIX_ADDTOLAYER + string::to_string(_layerID),
		COMMAND_PREFIX_ADDTOLAYER + string::to_string(_layerID)
	);

	GlobalCommandSystem().addCommand(
		COMMAND_PREFIX_MOVETOLAYER + string::to_string(_layerID),
		std::bind(&LayerCommandTarget::moveSelectionToLayer, this, std::placeholders::_1)
	);
	GlobalEventManager().addCommand(
		COMMAND_PREFIX_MOVETOLAYER + string::to_string(_layerID),
		COMMAND_PREFIX_MOVETOLAYER + string::to_string(_layerID)
	);

	GlobalCommandSystem().addCommand(
		COMMAND_PREFIX_SHOWLAYER + string::to_string(_layerID),
		std::bind(&LayerCommandTarget::showLayer, this, std::placeholders::_1)
	);
	GlobalEventManager().addCommand(
		COMMAND_PREFIX_SHOWLAYER + string::to_string(_layerID),
		COMMAND_PREFIX_SHOWLAYER + string::to_string(_layerID)
	);

	GlobalCommandSystem().addCommand(
		COMMAND_PREFIX_HIDELAYER + string::to_string(_layerID),
		std::bind(&LayerCommandTarget::hideLayer, this, std::placeholders::_1)
	);
	GlobalEventManager().addCommand(
		COMMAND_PREFIX_HIDELAYER + string::to_string(_layerID),
		COMMAND_PREFIX_HIDELAYER + string::to_string(_layerID)
	);
}

void LayerCommandTarget::addSelectionToLayer(const cmd::ArgumentList& args)
{
	if (!GlobalMapModule().getRoot())
	{
		rError() << "No map loaded, cannot do this." << std::endl;
		return;
	}

	// Pass the call to the LayerSystem
	GlobalMapModule().getRoot()->getLayerManager().addSelectionToLayer(_layerID);
}

void LayerCommandTarget::moveSelectionToLayer(const cmd::ArgumentList& args)
{
	if (!GlobalMapModule().getRoot())
	{
		rError() << "No map loaded, cannot do this." << std::endl;
		return;
	}

	// Pass the call to the LayerSystem
	GlobalMapModule().getRoot()->getLayerManager().moveSelectionToLayer(_layerID);
}

void LayerCommandTarget::showLayer(const cmd::ArgumentList& args) 
{
	if (!GlobalMapModule().getRoot())
	{
		rError() << "No map loaded, cannot do this." << std::endl;
		return;
	}

	GlobalMapModule().getRoot()->getLayerManager().setLayerVisibility(_layerID, true);
}

void LayerCommandTarget::hideLayer(const cmd::ArgumentList& args) 
{
	if (!GlobalMapModule().getRoot())
	{
		rError() << "No map loaded, cannot do this." << std::endl;
		return;
	}

	GlobalMapModule().getRoot()->getLayerManager().setLayerVisibility(_layerID, false);
}

} // namespace scene
