#include "MapPositionManager.h"

#include "maplib.h"
#include "ientity.h"
#include "icamera.h"
#include "gamelib.h"
#include "ieventmanager.h"
#include "iregistry.h"
#include "itextstream.h"
#include "imapresource.h"
#include "icommandsystem.h"
#include "string/string.h"
#include "entitylib.h"
#include <functional>

#include "map/Map.h"

namespace map
{

	namespace
	{
		const std::string SAVE_COMMAND_ROOT = "SavePosition";
		const std::string LOAD_COMMAND_ROOT = "LoadPosition";

		const char* const LAST_CAM_POSITION_KEY = "LastCameraPosition";
		const char* const LAST_CAM_ANGLE_KEY = "LastCameraAngle";

		const char* const GKEY_LAST_CAM_POSITION = "/mapFormat/lastCameraPositionKey";
		const char* const GKEY_LAST_CAM_ANGLE = "/mapFormat/lastCameraAngleKey";
		const char* const GKEY_PLAYER_START_ECLASS = "/mapFormat/playerStartPoint";
		const char* const GKEY_PLAYER_HEIGHT = "/defaults/playerHeight";

		const unsigned int MAX_POSITIONS = 10;

		bool tryLoadLastPositionFromMapRoot(Vector3& origin, Vector3& angles)
		{
			auto mapRoot = GlobalMapModule().getRoot();

			if (mapRoot)
			{
				const std::string savedOrigin = mapRoot->getProperty(LAST_CAM_POSITION_KEY);

				if (!savedOrigin.empty())
				{
					// Construct the vector out of the std::string
					origin = string::convert<Vector3>(savedOrigin);
					angles = string::convert<Vector3>(mapRoot->getProperty(LAST_CAM_ANGLE_KEY));
					return true;
				}
			}

			return false;
		}

		bool tryLoadLastPositionFromWorldspawn(Vector3& origin, Vector3& angles)
		{
			const std::string keyLastCamPos = game::current::getValue<std::string>(GKEY_LAST_CAM_POSITION);
			const std::string keyLastCamAngle = game::current::getValue<std::string>(GKEY_LAST_CAM_ANGLE);

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
					return true;
				}
			}

			return false;
		}

		bool tryGetStartPositionFromPlayerStart(Vector3& origin, Vector3& angles)
		{
			// Get the player start entity
			const std::string eClassPlayerStart = game::current::getValue<std::string>(GKEY_PLAYER_START_ECLASS);
			Entity* playerStart = Scene_FindEntityByClass(eClassPlayerStart);

			if (playerStart != nullptr)
			{
				// Get the entity origin
				origin = string::convert<Vector3>(playerStart->getKeyValue("origin"));

				// angua: move the camera upwards a bit
				origin.z() += game::current::getValue<float>(GKEY_PLAYER_HEIGHT);

				// Check for an angle key, and use it if present
				angles[ui::CAMERA_YAW] = string::convert<float>(playerStart->getKeyValue("angle"), 0);

				return true;
			}

			return false;
		}
	}

MapPositionManager::MapPositionManager()
{
	_mapEventConn = GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(this, &MapPositionManager::onMapEvent)
	);

	GlobalMapResourceManager().signal_onResourceExporting().connect(sigc::mem_fun(
		this, &MapPositionManager::onPreMapExport
	));

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
	}
}

MapPositionManager::~MapPositionManager()
{
	_mapEventConn.disconnect();
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

void MapPositionManager::removeLegacyCameraPosition()
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

void MapPositionManager::saveLastCameraPosition(const scene::IMapRootNodePtr& root)
{
	if (!root)
	{
		return;
	}

	try
	{
		auto& camView = GlobalCameraView().getActiveView();

		root->setProperty(LAST_CAM_POSITION_KEY, string::to_string(camView.getCameraOrigin()));
		root->setProperty(LAST_CAM_ANGLE_KEY, string::to_string(camView.getCameraAngles()));
	}
	catch (const std::runtime_error& ex)
	{
		rError() << "Cannot save last camera position: " << ex.what() << std::endl;
	}
}

void MapPositionManager::gotoLastCameraPosition()
{
	const std::string keyLastCamPos = game::current::getValue<std::string>(GKEY_LAST_CAM_POSITION);
	const std::string keyLastCamAngle = game::current::getValue<std::string>(GKEY_LAST_CAM_ANGLE);
	const std::string eClassPlayerStart = game::current::getValue<std::string>(GKEY_PLAYER_START_ECLASS);

	Vector3 angles(0, 0, 0);
	Vector3 origin(0, 0, 0);

	if (tryLoadLastPositionFromMapRoot(origin, angles) ||
		tryLoadLastPositionFromWorldspawn(origin, angles) ||
		tryGetStartPositionFromPlayerStart(origin, angles))
	{
		// Focus the view with the given parameters
		Map::focusViews(origin, angles);
		return;
	}
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
	case IMap::MapLoaded:
		// Load legacy positions from the worldspawn entity
		// and move them, from now on everything
		// will be stored to the root node's property bag
		convertLegacyPositions();

		// After converting the legacy ones, load the positions
		// from the map root, these take precedence
		loadMapPositions();

		gotoLastCameraPosition();

		// Remove any legacy keyvalues from worldspawn
		removeLegacyCameraPosition();
		break;

	case IMap::MapUnloaded:
		clearPositions();
		break;
	};
}

void MapPositionManager::onPreMapExport(const scene::IMapRootNodePtr& root)
{
	// Before any map is exported, save the last cam position to its root node
	saveLastCameraPosition(root);
}

} // namespace map
