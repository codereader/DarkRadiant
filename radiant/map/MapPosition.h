#ifndef MAPPOSITION_H_
#define MAPPOSITION_H_

#include <string>
#include "math/Vector3.h"
#include <boost/shared_ptr.hpp>

class Entity;

namespace map {

/** greebo: An instance of such a class manages a single Map Position.
 * 
 * It provides methods to save itself to a given target entity,
 * as well as methods to recall the stored position.
 */
class MapPosition
{
	unsigned int _index;
	
	// The saved position/angles
	Vector3 _position;
	Vector3 _angle;
	
	// The entity key names according to this index
	std::string _posKey;
	std::string _angleKey;

public:
	// Construct this position with its index
	MapPosition(unsigned int index);
	
	/** greebo: Loads the position from the given entity
	 */
	void load(Entity* entity);
	
	/** greebo: Saves the position to the given entity.
	 * 
	 * If this position is empty, the keys/values are 
	 * removed from the entity.
	 */
	void save(Entity* entity);
	
	/** greebo: Removes the values from the given entity
	 */
	void remove(Entity* entity);
	
	/** greebo: Resets the position/angles to 0,0,0
	 */
	void clear();
	
	/** greebo: Returns true if this position is not saved yet
	 */
	bool empty() const;
	
	/** greebo: Reads the current position from the camera and 
	 * stores it into the internal values.
	 */
	void store();
	
	/** greebo: Recalls the stored position
	 */
	void recall();
};

typedef boost::shared_ptr<MapPosition> MapPositionPtr;

} // namespace map

#endif /*MAPPOSITION_H_*/
