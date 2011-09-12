#pragma once

#include "iscenegraph.h"
#include "scenelib.h"

namespace map
{

class MapMergeAll :
	public scene::NodeVisitor
{
private:
	scene::Path _path;

public:
	MapMergeAll(const scene::Path& root) : 
		_path(root)
	{}

	bool pre(const scene::INodePtr& originalNode)
	{
		// The removeChildNode below might destroy the instance - push the refcount
		scene::INodePtr node = originalNode;

		// greebo: Un-register the node from its previous parent first to be clean
		scene::INodePtr oldParent = node->getParent();

		if (oldParent)
		{
			oldParent->removeChildNode(node);
		}

		_path.top()->addChildNode(node);
		_path.push(node);

		Node_setSelected(node, true);
		
		return false;
	}

	void post(const scene::INodePtr& node)
	{
		_path.pop();
	}
};

class MapMergeEntities :
	public scene::NodeVisitor
{
	// The target path (usually GlobalSceneGraph().root())
	mutable scene::Path m_path;

	scene::LayerList _targetLayers;

public:
	MapMergeEntities(const scene::Path& root) :
		m_path(root)
	{
		_targetLayers.insert(GlobalLayerSystem().getFirstVisibleLayer());
	}

	bool pre(const scene::INodePtr& originalNode)
	{
		// The removeChildNode below might destroy the instance - push the refcount
		scene::INodePtr node = originalNode;

		// greebo: Check if the visited node is the worldspawn of the other map
		if (node_is_worldspawn(node))
		{
			// Find the worldspawn of the target map
			scene::INodePtr world_node = GlobalMap().findWorldspawn();

			if (world_node == NULL)
			{
				// Set the worldspawn to the new node
				GlobalMap().setWorldspawn(node);

				// greebo: Un-register the node from its previous parent first to be clean
				scene::INodePtr oldParent = node->getParent();

				if (oldParent)
				{
					oldParent->removeChildNode(node);
				}

				// Insert the visited node at the target path
				m_path.top()->addChildNode(node);

				m_path.push(node);

				// Select all the children of the visited node (these are primitives)
				NodeSelector visitor;
				node->traverse(visitor);
			}
			else
			{
				// The target map already has a worldspawn
				m_path.push(world_node);

				// Merge all children of this node into the target worldspawn
				MapMergeAll visitor(m_path);
				node->traverse(visitor);
			}
		}
		else
		{
			// This is an ordinary entity, not worldspawn

			// Insert this node at the target path
			m_path.top()->addChildNode(node);
			m_path.push(node);

			// greebo: commented this out, we don't want the child brushes to be selected
			/*if (node_is_group(node)) {
				Node_getTraversable(node)->traverse(SelectChildren(m_path));
			}
			else {
				selectPath(m_path, true);
			}*/

			// Select the visited node
			Node_setSelected(node, true);
		}

		// Add the node to the target layer set
		scene::AssignNodeToLayersWalker walker(_targetLayers);
		Node_traverseSubgraph(node, walker);

		// Only traverse top-level entities, don't traverse the children
		return false;
	}

	virtual void post(const scene::INodePtr& node) {
		m_path.pop();
	}
};

/// Merges the map graph rooted at \p node into the global scene-graph.
inline void MergeMap(scene::INodePtr node)
{
	MapMergeEntities visitor(scene::Path(GlobalSceneGraph().root()));
	node->traverse(visitor);
}

} // namespace map
