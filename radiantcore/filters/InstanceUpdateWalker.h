#pragma once

#include "inode.h"
#include "ientity.h"
#include "iselectable.h"
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
 * Scenegraph walker to update filtered status of nodes based on the
 * currently active set of filters.
 */
class InstanceUpdateWalker :
	public scene::NodeVisitor
{
private:
	IFilterSystem& _filterSystem;

	// Helper visitors to update subgraphs
	NodeVisibilityUpdater _hideWalker;
	NodeVisibilityUpdater _showWalker;
	Deselector _deselector;

	// Cached boolean to avoid FilterSystem queries for each node
	bool _patchesAreVisible;
	bool _brushesAreVisible;

public:
	InstanceUpdateWalker(IFilterSystem& filterSystem) :
		_filterSystem(filterSystem),
		_hideWalker(true),
		_showWalker(false),
		_patchesAreVisible(_filterSystem.isVisible(FilterRule::TYPE_OBJECT, "patch")),
		_brushesAreVisible(_filterSystem.isVisible(FilterRule::TYPE_OBJECT, "brush"))
	{}

	bool pre(const scene::INodePtr& node) override
	{
		// Check entity eclass and spawnargs
		if (Node_isEntity(node))
		{
			bool isVisible = evaluateEntity(node);

			setSubgraphFilterStatus(node, isVisible);

			// If the entity is hidden, don't traverse its child nodes
			return isVisible;
		}

		// greebo: Check visibility of Patches
		if (Node_isPatch(node))
		{
			bool isVisible = evaluatePatch(node);

			setSubgraphFilterStatus(node, isVisible);
		}
		// greebo: Check visibility of Brushes
		else if (Node_isBrush(node))
		{
			bool isVisible = evaluateBrush(node);

			setSubgraphFilterStatus(node, isVisible);

			// In case the brush has at least one visible material trigger a fine-grained update
			if (isVisible)
			{
				Node_getIBrush(node)->updateFaceVisibility();
			}
		}

		// Continue the traversal
		return true;
	}

private:
	bool evaluateEntity(const scene::INodePtr& node)
	{
		assert(Node_isEntity(node));

		Entity* entity = Node_getEntity(node);

		// Check the eclass first
		return _filterSystem.isEntityVisible(FilterRule::TYPE_ENTITYCLASS, *entity) &&
			_filterSystem.isEntityVisible(FilterRule::TYPE_ENTITYKEYVALUE, *entity);
	}

	bool evaluatePatch(const scene::INodePtr& node)
	{
		assert(Node_isPatch(node));

		return _patchesAreVisible && Node_getIPatch(node)->hasVisibleMaterial();
	}

	bool evaluateBrush(const scene::INodePtr& node)
	{
		assert(Node_isBrush(node));

		return _brushesAreVisible && Node_getIBrush(node)->hasVisibleMaterial();
	}

	void setSubgraphFilterStatus(const scene::INodePtr& node, bool isVisible)
	{
		node->traverse(isVisible ? _showWalker : _hideWalker);

		if (!isVisible)
		{
			// de-select this node and all children
			node->traverse(_deselector);
		}
	}
};

} // namespace filters
