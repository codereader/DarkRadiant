#ifndef MERGE_H_
#define MERGE_H_

#include "iscenegraph.h"
#include "itraversable.h"

namespace map {

class MapMergeAll : 
	public scene::NodeVisitor
{
  mutable scene::Path m_path;
public:
  MapMergeAll(const scene::Path& root)
    : m_path(root)
  {
  }
  virtual bool pre(const scene::INodePtr& node)
  {
    m_path.top()->addChildNode(node);
    m_path.push(node);
    Node_setSelected(node, true);
    return false;
  }

  virtual void post(const scene::INodePtr& node) 
  {
    m_path.pop();
  }
};

class MapMergeEntities : 
	public scene::NodeVisitor
{
	// The target path (usually GlobalSceneGraph().root())
	mutable scene::Path m_path;

public:
	MapMergeEntities(const scene::Path& root) : 
		m_path(root) 
	{}

	virtual bool pre(const scene::INodePtr& node) {
		// greebo: Check if the visited node is the worldspawn of the other map
		if (node_is_worldspawn(node)) {
			// Find the worldspawn of the target map
			scene::INodePtr world_node = GlobalMap().findWorldspawn();
			
			if (world_node == NULL) {
				// Set the worldspawn to the new node
				GlobalMap().setWorldspawn(node);
				
				// Insert the visited node at the target path
				m_path.top()->addChildNode(node);
				
				m_path.push(node);
				
				// Select all the children of the visited node (these are primitives)
				NodeSelector visitor;
				node->traverse(visitor);
			}
			else {
				m_path.push(world_node);
				MapMergeAll visitor(m_path);
				node->traverse(visitor);
			}
		}
		else {
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
		
		// Only traverse top-level entities, don't traverse the children
		return false;
	}

  virtual void post(const scene::INodePtr& node)
  {
    m_path.pop();
  }
};

/// Merges the map graph rooted at \p node into the global scene-graph.
inline void MergeMap(scene::INodePtr node) {
	MapMergeEntities visitor(scene::Path(GlobalSceneGraph().root()));
	node->traverse(visitor);
}

} // namespace map

#endif /*MERGE_H_*/
