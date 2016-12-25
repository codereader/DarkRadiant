#pragma once

#include "iscenegraph.h"
#include "ientity.h"

namespace scene
{

// Tries to locate the named actor in the current map
class ActorNodeFinder :
	public scene::NodeVisitor
{
private:
	std::string _actorName;

	scene::INodePtr _foundNode;

public:
	ActorNodeFinder(const std::string& actorName) :
		_actorName(actorName)
	{}

	bool pre(const scene::INodePtr& node)
	{
		if (_foundNode) return false; // we've already found what we're looking for

		Entity* entity = Node_getEntity(node);

		if (entity == nullptr) return true;

		// Found an entity, compare names
		if (entity->getKeyValue("name") == _actorName)
		{
			_foundNode = node;
		}

		return false;
	}

	const scene::INodePtr& getFoundNode() const
	{
		return _foundNode;
	}
};

}
