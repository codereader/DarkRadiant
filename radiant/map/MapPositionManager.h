#pragma once

#include <map>
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

public:
	MapPositionManager();

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

	void saveLastCameraPosition();
	void removeLegacyCameraPosition();

	void onMapEvent(IMap::MapEvent ev);
};

} // namespace map
