#pragma once

#include "inode.h"
#include <wx/dataview.h>

namespace ui
{

/**
 * A structure representing a single scene node in the EntityList.
 * Holds a reference to the tree model's wxDataViewItem.
 */
class GraphTreeNode
{
private:
	// A reference to the actual node
	const scene::INodePtr& _node;

	// The iterator pointing to the row in a wxutil::TreeModel
	wxDataViewItem _iter;
public:
    using Ptr = std::shared_ptr<GraphTreeNode>;

    GraphTreeNode(const scene::INodePtr& node, const wxDataViewItem& iter) :
        _node(node),
        _iter(iter)
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

} // namespace
