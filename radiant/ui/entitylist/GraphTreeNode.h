#pragma once

#include "wxutil/TreeModel.h"

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

	// The iterator pointing to the row in a wxutil::TreeModel
	wxDataViewItem _iter;
public:
	GraphTreeNode(const scene::INodePtr& node) :
		_node(node)
	{}

	// Convenience accessor for methods
	wxDataViewItem& getIter()
	{
		return _iter;
	}

	const scene::INodePtr& getNode() const
	{
		return _node;
	}
};
typedef std::shared_ptr<GraphTreeNode> GraphTreeNodePtr;

} // namespace ui

