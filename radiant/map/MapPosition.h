#pragma once

#include <string>
#include "icommandsystem.h"
#include "imap.h"
#include "math/Vector3.h"
#include <memory>

class Entity;

namespace map 
{

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
	void loadFrom(Entity* entity);

	/** greebo: Removes the values from the given entity
	 */
	void removeFrom(Entity* entity);

	void loadFrom(const scene::IMapRootNodePtr& root);

	// Store the current position to the map root
	void saveTo(const scene::IMapRootNodePtr& root);

	// Remove the current position from the given node
	void removeFrom(const scene::IMapRootNodePtr& root);

	/** greebo: Resets the position/angles to 0,0,0
	 */
	void clear();

	/** greebo: Returns true if this position is not saved yet
	 */
	bool empty() const;

	/** greebo: Reads the current position from the camera and
	 * stores it into the internal values.
	 */
	void store(const cmd::ArgumentList& args);

	/** greebo: Recalls the stored position
	 */
	void recall(const cmd::ArgumentList& args);
};

typedef std::shared_ptr<MapPosition> MapPositionPtr;

} // namespace map
