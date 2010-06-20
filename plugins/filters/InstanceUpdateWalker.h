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

	// Cached boolean to avoid GlobalFilterSystem() queries for each node
	bool _patchesAreVisible;
	bool _brushesAreVisible;

public:
	InstanceUpdateWalker() :
		_hideWalker(true),
		_showWalker(false),
		_patchesAreVisible(GlobalFilterSystem().isVisible("object", "patch")),
		_brushesAreVisible(GlobalFilterSystem().isVisible("object", "brush"))
	{

	}

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
		
		// greebo: Update visibility of Patches
		IPatchNodePtr patchNode = boost::dynamic_pointer_cast<IPatchNode>(node);

		if (patchNode != NULL)
		{
			bool isVisible = _patchesAreVisible && patchNode->getPatch().hasVisibleMaterial();

			Node_traverseSubgraph(node, isVisible ? _showWalker : _hideWalker);
		}

		// greebo: Update visibility of Brushes
		IBrush* brush = Node_getIBrush(node);

		if (brush != NULL)
		{
			bool isVisible = _brushesAreVisible && brush->hasVisibleMaterial();

			Node_traverseSubgraph(node, isVisible ? _showWalker : _hideWalker);
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
