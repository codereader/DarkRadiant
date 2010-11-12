#ifndef GRAPHTREENODE_H_
#define GRAPHTREENODE_H_

#include <gtkmm/treemodel.h>

namespace ui
{

/**
 * greebo: This structure contains information about the
 *         scene::INode displayed in the EntityList.
 *
 * This includes a valid instance and a valid Gtk::TreeModel::iterator.
 */
class GraphTreeNode
{
private:
	// A reference to the actual node
	const scene::INodePtr& _node;

	// The GTK iterator pointing to the row in a Gtk::TreeStore
	Gtk::TreeModel::iterator _iter;
public:
	GraphTreeNode(const scene::INodePtr& node) :
		_node(node)
	{}

	// Convenience accessor for GTK methods (hence raw pointer)
	Gtk::TreeModel::iterator& getIter()
	{
		return _iter;
	}

	const scene::INodePtr& getNode() const {
		return _node;
	}
};
typedef boost::shared_ptr<GraphTreeNode> GraphTreeNodePtr;

} // namespace ui

#endif /*GRAPHTREENODE_H_*/
