#pragma once

#include <map>
#include <string>
#include "iscenegraph.h"
#include "ientity.h"
#include "ieclass.h"

namespace scene
{

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
	EntityBreakdown()
	{
		_map.clear();
		GlobalSceneGraph().root()->traverse(*this);
	}

	bool pre(const scene::INodePtr& node) override
	{
		// Is this node an entity?
		Entity* entity = Node_getEntity(node);

		if (entity != nullptr)
		{
			IEntityClassConstPtr eclass = entity->getEntityClass();
			std::string ecName = eclass->getDeclName();

			auto found = _map.find(ecName);

			if (found == _map.end())
			{
				// Entity class not yet registered, create new entry
				_map.emplace(ecName, 1);
			}
			else
			{
				// Eclass is known, increase the counter
				found->second++;
			}
		}

		return true;
	}

	// Accessor method to retrieve the entity breakdown map
	const Map& getMap() const
	{
		return _map;
	}

	Map::const_iterator begin() const
	{
		return _map.begin();
	}

	Map::const_iterator end() const
	{
		return _map.end();
	}

}; // class EntityBreakdown

} // namespace
