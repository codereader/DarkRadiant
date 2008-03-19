#ifndef ENTITYBREAKDOWN_H_
#define ENTITYBREAKDOWN_H_

#include <map>
#include <string>
#include "iscenegraph.h"
#include "ientity.h"
#include "ieclass.h"

namespace map {

/** greebo: This object traverses the scenegraph on construction
 * 			counting all occurrences of each entity class. 
 */
class EntityBreakdown :
	public scene::Graph::Walker
{
public:
	typedef std::map<std::string, std::size_t> Map;

private:
	mutable Map _map;

public:
	EntityBreakdown() {
		_map.clear();
		GlobalSceneGraph().traverse(*this);
	}
	
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		// Is this node an entity?
		Entity* entity = Node_getEntity(path.top());
		if (entity != NULL) {
			IEntityClassConstPtr eclass = entity->getEntityClass();
			std::string ecName = eclass->getName();
      
			if (_map.find(ecName) == _map.end()) {
				// Entity class not yet registered, create new entry
				_map[ecName] = 1;
			}
			else {
				// Eclass is known, increase the counter
				_map[ecName]++;
			}
		}
		
		return true;
	}
	
	// Accessor method to retrieve the entity breakdown map
	Map getMap() {
		return _map;
	}
	
	Map::iterator begin() {
		return _map.begin();
	}
	
	Map::iterator end() {
		return _map.end();
	}

}; // class EntityBreakdown

} // namespace map

#endif /*ENTITYBREAKDOWN_H_*/
