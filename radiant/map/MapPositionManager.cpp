#include "MapPositionManager.h"

#include "ientity.h"
#include "ieventmanager.h"
#include "iregistry.h"
#include "itextstream.h"
#include "icommandsystem.h"
#include "string/string.h"
#include <functional>

namespace map
{

	namespace
	{
		const std::string SAVE_COMMAND_ROOT = "SavePosition";
		const std::string LOAD_COMMAND_ROOT = "LoadPosition";

		unsigned int MAX_POSITIONS = 10;
	}

// Constructor
MapPositionManager::MapPositionManager()
{
	GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(this, &MapPositionManager::onMapEvent)
	);

	// Create the MapPosition objects and add the commands to the eventmanager
	for (unsigned int i = 1; i <= MAX_POSITIONS; i++)
	{
		// Allocate a new MapPosition object and store the shared_ptr
		_positions[i] = std::make_shared<MapPosition>(i);

		// Add the load/save commands to the eventmanager and point it to the member
		GlobalCommandSystem().addCommand(
			SAVE_COMMAND_ROOT + string::to_string(i),
			std::bind(&MapPosition::store, _positions[i].get(), std::placeholders::_1)
		);
		GlobalCommandSystem().addCommand(
			LOAD_COMMAND_ROOT + string::to_string(i),
			std::bind(&MapPosition::recall, _positions[i].get(), std::placeholders::_1)
		);

		GlobalEventManager().addCommand(
			SAVE_COMMAND_ROOT + string::to_string(i),
			SAVE_COMMAND_ROOT + string::to_string(i)
		);

		GlobalEventManager().addCommand(
			LOAD_COMMAND_ROOT + string::to_string(i),
			LOAD_COMMAND_ROOT + string::to_string(i)
		);
	}
}

void MapPositionManager::loadPositions()
{
	// Find the worldspawn node
	const scene::INodePtr& wsNode = GlobalMapModule().getWorldspawn();

	if (!wsNode) return;

	Entity* worldspawn = Node_getEntity(wsNode);

	if (worldspawn != nullptr) 
	{
		for (unsigned int i = 1; i <= MAX_POSITIONS; ++i)
		{
			if (_positions[i])
			{
				_positions[i]->load(worldspawn);
			}
		}
	}
	else
	{
		rError() << "MapPositionManager: Could not locate worldspawn entity.\n";
	}
}

void MapPositionManager::savePositions()
{
	// Find the worldspawn node
	const scene::INodePtr& wsNode = GlobalMapModule().getWorldspawn();

	if (!wsNode) return;

	Entity* worldspawn = Node_getEntity(wsNode);

	for (unsigned int i = 1; i <= MAX_POSITIONS; ++i)
	{
		if (_positions[i])
		{
			_positions[i]->save(worldspawn);
		}
	}
}

void MapPositionManager::removePositions()
{
	// Find the worldspawn node
	const scene::INodePtr& wsNode = GlobalMapModule().getWorldspawn();

	if (!wsNode) return;

	Entity* worldspawn = Node_getEntity(wsNode);

	for (unsigned int i = 1; i <= MAX_POSITIONS; ++i) 
	{
		if (_positions[i])
		{
			_positions[i]->remove(worldspawn);
		}
	}
}

void MapPositionManager::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapSaving)
	{
		// Store the map positions into the worldspawn spawnargs
		savePositions();
	}
	else if (ev == IMap::MapSaved)
	{
		// Remove the map positions again after saving
		removePositions();
	}
	else if (ev == IMap::MapLoaded)
	{
		// Load the stored map positions from the worldspawn entity
		loadPositions();
		// Remove them, so that the user doesn't get bothered with them
		removePositions();
	}
}

} // namespace map
