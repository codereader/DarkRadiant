#ifndef CLONEALLWALKER_H_
#define CLONEALLWALKER_H_

#include "inode.h"
#include "itraversable.h"

namespace map {

inline scene::CloneablePtr Node_getCloneable(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<scene::Cloneable>(node);
}

inline scene::INodePtr node_clone(scene::INodePtr node) {
	scene::CloneablePtr cloneable = Node_getCloneable(node);
	if (cloneable != NULL) {
		return cloneable->clone();
	}
  
	// Return an empty node
	return scene::INodePtr(new scene::Node);
}

class CloneAll : 
	public scene::Traversable::Walker
{
	mutable scene::Path m_path;
public:
	CloneAll(scene::INodePtr root) : 
		m_path(root)
	{}
	
  bool pre(scene::INodePtr node) const
  {
    if (node->isRoot()) {
      return false;
    }
    
    m_path.push(node_clone(node));

    return true;
  }
  void post(scene::INodePtr node) const
  {
    if(node->isRoot()) {
      return;
    }

    Node_getTraversable(m_path.parent())->insert(m_path.top());

    m_path.pop();
  }
};

inline scene::INodePtr Node_Clone(scene::INodePtr node)
{
  scene::INodePtr clone = node_clone(node);
  scene::TraversablePtr traversable = Node_getTraversable(node);
  if(traversable != NULL)
  {
    traversable->traverse(CloneAll(clone));
  }
  return clone;
}

} // namespace map

#endif /*CLONEALLWALKER_H_*/
