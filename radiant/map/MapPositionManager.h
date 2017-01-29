#pragma once

#include <map>
#include "imap.h"
#include "MapPosition.h"

namespace map {

class MapPositionManager
{
private:
	// The position map (with a key as integer)
	typedef std::map<unsigned int, MapPositionPtr> PositionMap;

	PositionMap _positions;

public:
	MapPositionManager();

	/** greebo: This loads/saves the positions from
	 * the worldspawn entity. Call this on map load/save
	 */
	void loadPositions();
	void savePositions();

	/** greebo: This cleans the positions from the
	 * worldspawn entity (should be called after a map load
	 * to clean the entity manager from these entries).
	 */
	void removePositions();

private:
	void onMapEvent(IMap::MapEvent ev);
};

} // namespace map
