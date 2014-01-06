#pragma once

#include "iscenegraph.h"
#include "ientity.h"
#include "iselectable.h"
#include "ieclass.h"
#include "ipatch.h"
#include "ibrush.h"

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
		_patchesAreVisible(GlobalFilterSystem().isVisible(FilterRule::TYPE_OBJECT, "patch")),
		_brushesAreVisible(GlobalFilterSystem().isVisible(FilterRule::TYPE_OBJECT, "brush"))
	{

	}

	// Pre-descent walker function
	bool pre(const scene::INodePtr& node)
	{
		// Retrieve the parent entity and check its entity class.
		Entity* entity = Node_getEntity(node);

		if (entity != NULL)
		{
			// Check the eclass first
			bool entityIsVisible = GlobalFilterSystem().isEntityVisible(FilterRule::TYPE_ENTITYCLASS, *entity) &&
								   GlobalFilterSystem().isEntityVisible(FilterRule::TYPE_ENTITYKEYVALUE, *entity);

			node->traverse(entityIsVisible ? _showWalker : _hideWalker);

			// If the entity is hidden, don't traverse the child nodes
			return entityIsVisible;
		}

		// greebo: Update visibility of Patches
		IPatchNodePtr patchNode = boost::dynamic_pointer_cast<IPatchNode>(node);

		if (patchNode != NULL)
		{
			bool isVisible = _patchesAreVisible && patchNode->getPatch().hasVisibleMaterial();

			node->traverse(isVisible ? _showWalker : _hideWalker);
		}

		// greebo: Update visibility of Brushes
		IBrush* brush = Node_getIBrush(node);

		if (brush != NULL)
		{
			bool isVisible = _brushesAreVisible && brush->hasVisibleMaterial();

			node->traverse(isVisible ? _showWalker : _hideWalker);

			// In case the brush has at least one visible material trigger a fine-grained update
			if (isVisible)
			{
				brush->updateFaceVisibility();
			}
		}

		if (!node->visible())
		{
			// de-select this node and all children
			Deselector deselector;
			node->traverse(deselector);
		}

		// Continue the traversal
		return true;
	}
};

} // namespace filters
