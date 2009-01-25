#ifndef GRAPHTREEMODELPOPULATOR_H_
#define GRAPHTREEMODELPOPULATOR_H_

#include "iscenegraph.h"
#include "scenelib.h"
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
	// The model to be populated
	GraphTreeModel& _model;

public:
	GraphTreeModelPopulator(GraphTreeModel& model) :
		_model(model)
	{
		// Clear out the model before traversal
		_model.clear();
	}
	
	// NodeVisitor implementation
	bool pre(const scene::INodePtr& node) {
		// Insert this node into the GraphTreeModel
		_model.insert(node);
		
		Entity* ent = Node_getEntity(node);

		if (ent != NULL && ent->getKeyValue("classname") == "worldspawn") {
			// Don't accumulate the worldspawn brushes
			return false;
		}

		return true; // traverse children
	}
};

} // namespace ui

#endif /*GRAPHTREEMODELPOPULATOR_H_*/
