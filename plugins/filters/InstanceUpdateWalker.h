#ifndef INSTANCEUPDATEWALKER_H_
#define INSTANCEUPDATEWALKER_H_

#include "iscenegraph.h"
#include "ientity.h"
#include "ieclass.h"
#include "ipatch.h"

#include "scenelib.h"

namespace filters {

// Walker: de-selects a complete subgraph
class Deselector :
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node) {
		Node_setSelected(node, false);
		return true;
	}
};

// Walker: Shows or hides a complete subgraph
class NodeVisibilityUpdater :
	public scene::NodeVisitor
{
private:
	bool _filtered;

public:
	NodeVisibilityUpdater(bool setFiltered) :
		_filtered(setFiltered)
	{}

	bool pre(const scene::INodePtr& node)
	{
		node->setFiltered(_filtered);
		return true;
	}
};

/**
 * Scenegraph walker to update filtered status of Instances based on the
 * status of their parent entity class.
 */
class InstanceUpdateWalker : 
	public scene::NodeVisitor
{
private:
	// Helper visitors to update subgraphs
	NodeVisibilityUpdater _hideWalker;
	NodeVisibilityUpdater _showWalker;
public:
	InstanceUpdateWalker() :
		_hideWalker(true),
		_showWalker(false)
	{}

	// Pre-descent walker function
	bool pre(const scene::INodePtr& node)
	{
		// Retrieve the parent entity and check its entity class.
		Entity* entity = Node_getEntity(node);
		if (entity != NULL)
		{
			IEntityClassConstPtr eclass = entity->getEntityClass();
			bool entityClassVisible = GlobalFilterSystem().isVisible("entityclass", eclass->getName());

			Node_traverseSubgraph(
				node, 
				entityClassVisible ? _showWalker : _hideWalker
			);

			// If the entity class is hidden, don't traverse the child nodes
			return entityClassVisible;
		}
		
		// greebo: Update visibility of PatchInstances
		if (Node_isPatch(node))
		{
			Node_traverseSubgraph(
				node, 
				GlobalFilterSystem().isVisible("object", "patch") ? _showWalker : _hideWalker
			);
		}
		
		// greebo: Update visibility of BrushInstances
		if (Node_isBrush(node))
		{
			Node_traverseSubgraph(
				node, 
				GlobalFilterSystem().isVisible("object", "brush") ? _showWalker : _hideWalker
			);
		}

		if (!node->visible())
		{
			// de-select this node and all children
			Deselector deselector;
			Node_traverseSubgraph(node, deselector);
		}

		// Continue the traversal
		return true;
	}
};

} // namespace filters

#endif /*INSTANCEUPDATEWALKER_H_*/
