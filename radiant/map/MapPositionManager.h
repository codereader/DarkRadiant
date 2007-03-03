#ifndef MAPPOSITIONMANAGER_H_
#define MAPPOSITIONMANAGER_H_

#include <map>
#include "MapPosition.h"

namespace map {

class MapPositionManager
{
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
	
	/** greebo: This adds all the commands
	 * to the Eventmanager (SavePosition1, LoadPosition1, etc.);
	 */
	void initialise();
};

// The accessor function of the MapPositionManager
MapPositionManager& GlobalMapPosition();

} // namespace map

#endif /*MAPPOSITIONMANAGER_H_*/
