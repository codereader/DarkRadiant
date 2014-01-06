#pragma once

#include "iscenegraph.h"
#include "ientity.h"
#include "GraphTreeModel.h"

namespace ui {

/**
 * greebo: The purpose of this class is to traverse the entire scenegraph and
 *         push all the found nodes into the given GraphTreeModel.
 *
 * This is used by the GraphTreeModel itself to update its status on show.
 */
class GraphTreeModelPopulator :
	public scene::NodeVisitor
{
private:
	// The model to be populated
	GraphTreeModel& _model;

	bool _visibleNodesOnly;

public:
	GraphTreeModelPopulator(GraphTreeModel& model, bool visibleNodesOnly) :
		_model(model),
		_visibleNodesOnly(visibleNodesOnly)
	{
		// Clear out the model before traversal
		_model.clear();
	}

	// NodeVisitor implementation
	bool pre(const scene::INodePtr& node)
	{
		if (!_visibleNodesOnly || node->visible())
		{
			// Insert this node into the GraphTreeModel
			_model.insert(node);
		}

		Entity* ent = Node_getEntity(node);

		if (ent != NULL && ent->getKeyValue("classname") == "worldspawn") {
			// Don't accumulate the worldspawn brushes
			return false;
		}

		return true; // traverse children
	}
};

} // namespace ui
