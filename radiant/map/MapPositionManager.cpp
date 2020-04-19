#include "MapPositionManager.h"

#include "maplib.h"
#include "ientity.h"
#include "gamelib.h"
#include "ieventmanager.h"
#include "iregistry.h"
#include "itextstream.h"
#include "icommandsystem.h"
#include "string/string.h"
#include "entitylib.h"
#include <functional>

#include "camera/GlobalCamera.h"
#include "xyview/GlobalXYWnd.h"
#include "map/algorithm/MapExporter.h"

namespace map
{

	namespace
	{
		const std::string SAVE_COMMAND_ROOT = "SavePosition";
		const std::string LOAD_COMMAND_ROOT = "LoadPosition";

		const char* const GKEY_LAST_CAM_POSITION = "/mapFormat/lastCameraPositionKey";
		const char* const GKEY_LAST_CAM_ANGLE = "/mapFormat/lastCameraAngleKey";
		const char* const GKEY_PLAYER_START_ECLASS = "/mapFormat/playerStartPoint";
		const char* const GKEY_PLAYER_HEIGHT = "/defaults/playerHeight";

		const unsigned int MAX_POSITIONS = 10;
	}

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

void MapPositionManager::convertLegacyPositions()
{
	Entity* worldspawn = map::current::getWorldspawn();
	auto mapRoot = GlobalMapModule().getRoot();

	if (worldspawn == nullptr || !mapRoot)
	{
		return; // no worldspawn or root
	}
		
	for (unsigned int i = 1; i <= MAX_POSITIONS; i++)
	{
		MapPosition pos(i);

		pos.loadFrom(worldspawn);

		if (!pos.empty() && mapRoot)
		{
			rMessage() << "Converting legacy map position #" << i << std::endl;
			pos.saveTo(mapRoot);

			pos.removeFrom(worldspawn);
		}
	}
}

void MapPositionManager::removeLastCameraPosition()
{
	const std::string keyLastCamPos = game::current::getValue<std::string>(GKEY_LAST_CAM_POSITION);
	const std::string keyLastCamAngle = game::current::getValue<std::string>(GKEY_LAST_CAM_ANGLE);

	Entity* worldspawn = map::current::getWorldspawn();

	if (worldspawn != nullptr)
	{
		worldspawn->setKeyValue(keyLastCamPos, "");
		worldspawn->setKeyValue(keyLastCamAngle, "");
	}
}

void MapPositionManager::saveLastCameraPosition()
{
	const std::string keyLastCamPos = game::current::getValue<std::string>(GKEY_LAST_CAM_POSITION);
	const std::string keyLastCamAngle = game::current::getValue<std::string>(GKEY_LAST_CAM_ANGLE);

	Entity* worldspawn = map::current::getWorldspawn();

	if (worldspawn != nullptr)
	{
		ui::CamWndPtr camWnd = GlobalCamera().getActiveCamWnd();
		if (camWnd == NULL) return;

		worldspawn->setKeyValue(keyLastCamPos, string::to_string(camWnd->getCameraOrigin()));
		worldspawn->setKeyValue(keyLastCamAngle, string::to_string(camWnd->getCameraAngles()));
	}
}

void MapPositionManager::gotoLastCameraPosition()
{
	const std::string keyLastCamPos = game::current::getValue<std::string>(GKEY_LAST_CAM_POSITION);
	const std::string keyLastCamAngle = game::current::getValue<std::string>(GKEY_LAST_CAM_ANGLE);
	const std::string eClassPlayerStart = game::current::getValue<std::string>(GKEY_PLAYER_START_ECLASS);

	Vector3 angles(0, 0, 0);
	Vector3 origin(0, 0, 0);

	Entity* worldspawn = map::current::getWorldspawn();

	if (worldspawn != nullptr)
	{
		// Try to find a saved "last camera position"
		const std::string savedOrigin = worldspawn->getKeyValue(keyLastCamPos);

		if (!savedOrigin.empty())
		{
			// Construct the vector out of the std::string
			origin = string::convert<Vector3>(savedOrigin);
			angles = string::convert<Vector3>(worldspawn->getKeyValue(keyLastCamAngle));
		}
		else
		{
			// Get the player start entity
			Entity* playerStart = Scene_FindEntityByClass(eClassPlayerStart);

			if (playerStart != NULL)
			{
				// Get the entity origin
				origin = string::convert<Vector3>(playerStart->getKeyValue("origin"));

				// angua: move the camera upwards a bit
				origin.z() += game::current::getValue<float>(GKEY_PLAYER_HEIGHT);

				// Check for an angle key, and use it if present
				angles[ui::CAMERA_YAW] = string::convert<float>(playerStart->getKeyValue("angle"), 0);
			}
		}
	}

	// Focus the view with the given parameters
	GlobalCamera().focusCamera(origin, angles);
	GlobalXYWnd().setOrigin(origin);
}

void MapPositionManager::loadMapPositions()
{
	auto mapRoot = GlobalMapModule().getRoot();

	if (!mapRoot)
	{
		return;
	}

	for (const auto& position : _positions)
	{
		position.second->loadFrom(mapRoot);
	}
}

void MapPositionManager::clearPositions()
{
	for (const auto& position : _positions)
	{
		position.second->clear();
	}
}

void MapPositionManager::onMapEvent(IMap::MapEvent ev)
{
	switch (ev)
	{
	case IMap::MapSaving:
		// Store the last map position into the worldspawn spawnargs
		saveLastCameraPosition();
		break;

	case IMap::MapSaved:
		// Remove the map positions again after saving
		removeLastCameraPosition();
		break;

	case IMap::MapLoaded:
		// Load legacy positions from the worldspawn entity
		// and move them, from now on everything
		// will be stored to the root node's property bag
		convertLegacyPositions();

		// After converting the legacy ones, load the positions
		// from the map root, these take precedence
		loadMapPositions();

		gotoLastCameraPosition();
		removeLastCameraPosition();
		break;

	case IMap::MapUnloaded:
		clearPositions();
		break;
	};
}

} // namespace map
