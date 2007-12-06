#ifndef GRAPHTREEMODELPOPULATOR_H_
#define GRAPHTREEMODELPOPULATOR_H_

#include "iscenegraph.h"
#include "GraphTreeModel.h"

namespace ui {

/**
 * greebo: The purpose of this class is to traverse the entire scenegraph and
 *         push all the found instances into the given GraphTreeModel.
 * 
 * This is used by the GraphTreeModel itself to update its status on show.
 */
class GraphTreeModelPopulator :
	public scene::Graph::Walker
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
	
	// Graph::Walker implementation
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		// Insert this instance into the GraphTreeModel
		_model.insert(instance);
		
		return true; // traverse children
	}
};

} // namespace ui

#endif /*GRAPHTREEMODELPOPULATOR_H_*/
