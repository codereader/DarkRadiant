#ifndef ENTITYBREAKDOWN_H_
#define ENTITYBREAKDOWN_H_

#include <map>
#include <string>
#include "scenelib.h"
#include "ientity.h"
#include "ieclass.h"

namespace map {

/** greebo: This object traverses the scenegraph on construction
 * 			counting all occurrences of each entity class. 
 */
class EntityBreakdown :
	public scene::NodeVisitor
{
public:
	typedef std::map<std::string, std::size_t> Map;

private:
	Map _map;

public:
	EntityBreakdown() {
		_map.clear();
		Node_traverseSubgraph(GlobalSceneGraph().root(), *this);
	}
	
	bool pre(const scene::INodePtr& node) {
		// Is this node an entity?
		Entity* entity = Node_getEntity(node);

		if (entity != NULL) {
			IEntityClassConstPtr eclass = entity->getEntityClass();
			std::string ecName = eclass->getName();
      
			Map::iterator found = _map.find(ecName);
			if (found == _map.end()) {
				// Entity class not yet registered, create new entry
				_map.insert(Map::value_type(ecName, 1));
			}
			else {
				// Eclass is known, increase the counter
				found->second++;
			}
		}
		
		return true;
	}
	
	// Accessor method to retrieve the entity breakdown map
	Map getMap() {
		return _map;
	}
	
	Map::const_iterator begin() const {
		return _map.begin();
	}
	
	Map::const_iterator end() const {
		return _map.end();
	}

}; // class EntityBreakdown

} // namespace map

#endif /*ENTITYBREAKDOWN_H_*/
