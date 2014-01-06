#pragma once

#include "ientity.h"
#include "gamelib.h"
#include "DifficultySettings.h"

namespace difficulty {

class DifficultyEntityFinder :
	public scene::NodeVisitor
{
public:
	// Found difficulty entities are stored in this list
	typedef std::vector<Entity*> EntityList;

private:
	// The entityClass name to search for
	std::string _entityClassName;

	// The list of found entities
	EntityList _foundEntities;

public:
	DifficultyEntityFinder() :
		_entityClassName(game::current::getValue<std::string>(GKEY_DIFFICULTY_ENTITYDEF_MAP))
	{}

	// Return the list of found entites
	const EntityList& getEntities() const {
		return _foundEntities;
	}

	// Walker implementation
	bool pre(const scene::INodePtr& node) {
		// Check if we have an entity
		Entity* entity = Node_getEntity(node);

		if (entity != NULL) {
			// Check for a classname match
			if (entity->getKeyValue("classname") == _entityClassName) {
				_foundEntities.push_back(entity);
			}

			// Don't traverse the children of entities
			return false;
		}

		return true;
	}
};

} // namespace difficulty
