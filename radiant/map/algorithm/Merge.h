#ifndef MERGE_H_
#define MERGE_H_

#include "iscenegraph.h"
#include "itraversable.h"

namespace map {

class MapMergeAll : public scene::Traversable::Walker
{
  mutable scene::Path m_path;
public:
  MapMergeAll(const scene::Path& root)
    : m_path(root)
  {
  }
  bool pre(scene::INodePtr node) const
  {
    Node_getTraversable(m_path.top())->insert(node);
    m_path.push(node);
    selectPath(m_path, true);
    return false;
  }
  void post(scene::INodePtr node) const
  {
    m_path.pop();
  }
};

class MapMergeEntities : 
	public scene::Traversable::Walker
{
	// The target path (usually GlobalSceneGraph().root())
	mutable scene::Path m_path;

public:
	MapMergeEntities(const scene::Path& root) : 
		m_path(root) 
	{}

	bool pre(scene::INodePtr node) const {
		// greebo: Check if the visited node is the worldspawn of the other map
		if (node_is_worldspawn(node)) {
			// Find the worldspawn of the target map
			scene::INodePtr world_node = GlobalMap().findWorldspawn();
			
			if (world_node == NULL) {
				// Set the worldspawn to the new node
				GlobalMap().setWorldspawn(node);
				
				// Insert the visited node at the target path
				Node_getTraversable(m_path.top())->insert(node);
				
				m_path.push(node);
				
				// Select all the children of the visited node (these are primitives)
				Node_getTraversable(node)->traverse(SelectChildren(m_path));
			}
			else {
				m_path.push(world_node);
				Node_getTraversable(node)->traverse(MapMergeAll(m_path));
			}
		}
		else {
			// This is an ordinary entity, not worldspawn
			
			// Insert this node at the target path 
			Node_getTraversable(m_path.top())->insert(node);
			m_path.push(node);
			
			// greebo: commented this out, we don't want the child brushes to be selected
			/*if (node_is_group(node)) {
				Node_getTraversable(node)->traverse(SelectChildren(m_path));
			}
			else {
				selectPath(m_path, true);
			}*/
			
			// Select the visited instance
			selectPath(m_path, true);
		}
		
		// Only traverse top-level entities, don't traverse the children
		return false;
	}

  void post(scene::INodePtr node) const
  {
    m_path.pop();
  }
};

/// Merges the map graph rooted at \p node into the global scene-graph.
inline void MergeMap(scene::INodePtr node) {
	Node_getTraversable(node)->traverse(
		MapMergeEntities(scene::Path(GlobalSceneGraph().root()))
	);
}

} // namespace map

#endif /*MERGE_H_*/
