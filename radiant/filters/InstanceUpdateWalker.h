#pragma once

#include "iscenegraph.h"
#include "ientity.h"
#include "iselectable.h"
#include "ieclass.h"
#include "ipatch.h"
#include "ibrush.h"

namespace filters 
{

// Walker: de-selects a complete subgraph
class Deselector :
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node) override
	{
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

	bool pre(const scene::INodePtr& node) override
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
	FilterSystem& _filterSystem;

	// Helper visitors to update subgraphs
	NodeVisibilityUpdater _hideWalker;
	NodeVisibilityUpdater _showWalker;
	Deselector _deselector;

	// Cached boolean to avoid FilterSystem queries for each node
	bool _patchesAreVisible;
	bool _brushesAreVisible;

public:
	InstanceUpdateWalker(FilterSystem& filterSystem) :
		_filterSystem(filterSystem),
		_hideWalker(true),
		_showWalker(false),
		_patchesAreVisible(_filterSystem.isVisible(FilterRule::TYPE_OBJECT, "patch")),
		_brushesAreVisible(_filterSystem.isVisible(FilterRule::TYPE_OBJECT, "brush"))
	{}

	// Pre-descent walker function
	bool pre(const scene::INodePtr& node) override
	{
		// Check entity eclass and spawnargs
		if (Node_isEntity(node))
		{
			Entity* entity = Node_getEntity(node);

			// Check the eclass first
			bool entityIsVisible = _filterSystem.isEntityVisible(FilterRule::TYPE_ENTITYCLASS, *entity) &&
								   _filterSystem.isEntityVisible(FilterRule::TYPE_ENTITYKEYVALUE, *entity);

			node->traverse(entityIsVisible ? _showWalker : _hideWalker);

			if (!entityIsVisible)
			{
				// de-select this node and all children
				node->traverse(_deselector);
			}

			// If the entity is hidden, don't traverse the child nodes
			return entityIsVisible;
		}

		// greebo: Update visibility of Patches
		if (Node_isPatch(node))
		{
			auto patchNode = std::dynamic_pointer_cast<IPatchNode>(node);

			bool isVisible = _patchesAreVisible && patchNode->getPatch().hasVisibleMaterial();

			node->traverse(isVisible ? _showWalker : _hideWalker);
		}
		// greebo: Update visibility of Brushes
		else if (Node_isBrush(node))
		{
			auto brush = Node_getIBrush(node);

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
			node->traverse(_deselector);
		}

		// Continue the traversal
		return true;
	}
};

} // namespace filters
