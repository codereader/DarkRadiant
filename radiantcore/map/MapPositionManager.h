#pragma once

#include <map>
#include <sigc++/connection.h>
#include "imap.h"
#include "MapPosition.h"

namespace map
{

class MapPositionManager
{
private:
	// The position map (with a key as integer)
	typedef std::map<unsigned int, MapPositionPtr> PositionMap;

	PositionMap _positions;

	sigc::connection _mapEventConn;

public:
	MapPositionManager();
	~MapPositionManager();

private:
	void convertLegacyPositions();
	void loadMapPositions();
	void clearPositions();

	/** 
	* greebo: Sets the camera to the start position. This uses the 
	* information stored in the worlspawn or the location of the 
	* info_player_start entity. If neither of these two exist, 0,0,0 is used.
	 */
	void gotoLastCameraPosition();

	void saveLastCameraPosition(const scene::IMapRootNodePtr& root);
	void removeLegacyCameraPosition();

	void onMapEvent(IMap::MapEvent ev);

	void onPreMapExport(const scene::IMapRootNodePtr& root);
};

} // namespace map
