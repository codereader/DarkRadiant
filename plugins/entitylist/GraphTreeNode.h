#ifndef GRAPHTREENODE_H_
#define GRAPHTREENODE_H_

namespace ui {

/**
 * greebo: This structure contains information about the
 *         scene::INode displayed in the EntityList.
 * 
 * This includes a valid instance and a valid GtkTreeIter.
 */
class GraphTreeNode {
	// A reference to the actual node
	const scene::INodePtr& _node;
	
	// The GTK iterator pointing to the row in a GtkTreeStore
	GtkTreeIter _iter;
public:
	GraphTreeNode(const scene::INodePtr& node) :
		_node(node)
	{}
	
	// Convenience accessor for GTK methods (hence raw pointer)
	GtkTreeIter* getIter() {
		return &_iter;
	}
	
	const scene::INodePtr& getNode() const {
		return _node;
	}
};
typedef boost::shared_ptr<GraphTreeNode> GraphTreeNodePtr;

} // namespace ui

#endif /*GRAPHTREENODE_H_*/
