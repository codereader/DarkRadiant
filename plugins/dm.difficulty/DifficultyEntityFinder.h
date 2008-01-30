#ifndef DIFFICULTY_ENTITY_H_
#define DIFFICULTY_ENTITY_H_

#include "ientity.h"
#include "scenelib.h"
#include "DifficultySettings.h"

namespace difficulty {

class DifficultyEntityFinder :
	public scene::Graph::Walker
{
public:
	// Found difficulty entities are stored in this list
	typedef std::vector<Entity*> EntityList;

private:
	// The entityClass name to search for
	std::string _entityClassName;

	// The list of found entities
	mutable EntityList _foundEntities;

public:
	DifficultyEntityFinder() :
		_entityClassName(GlobalRegistry().get(RKEY_DIFFICULTY_ENTITYDEF_MAP))
	{}

	// Return the list of found entites
	const EntityList& getEntities() const {
		return _foundEntities;
	}

	// Walker implementation
	virtual bool pre(const scene::Path& path, scene::Instance& instance) const {
		// Entities have path depth == 2
		if (path.size() >= 2) {
			// Check if we have an entity
			Entity* entity = Node_getEntity(path.top());

			// Check if we have a matching entity class
			if (entity != NULL && 
				entity->getKeyValue("classname") == _entityClassName)
			{
				_foundEntities.push_back(entity);
			}
			
			// Don't traverse the children of entities
			return false;
		}

		return true;
	}
};

} // namespace difficulty

#endif /* DIFFICULTY_ENTITY_H_ */
