#include "GraphTreeModel.h"

#include <iostream>
#include "iselectable.h"
#include "iselection.h"

#include "GraphTreeModelPopulator.h"

namespace ui
{

GraphTreeModel::GraphTreeModel() :
	_model(new wxutil::TreeModel(_columns)),
	_visibleNodesOnly(false)
{}

GraphTreeModel::~GraphTreeModel()
{
	// Remove everything before shutting down
	clear();
}

void GraphTreeModel::connectToSceneGraph()
{
	// Subscribe to the scenegraph to get notified about insertions/deletions
	GlobalSceneGraph().addSceneObserver(this);
}

void GraphTreeModel::disconnectFromSceneGraph()
{
	GlobalSceneGraph().removeSceneObserver(this);
}

const GraphTreeNodePtr& GraphTreeModel::insert(const scene::INodePtr& node)
{
	// Create a new GraphTreeNode
	GraphTreeNodePtr gtNode(new GraphTreeNode(node));

	// Insert this iterator below a possible parent iterator
	wxDataViewItem parentIter = findParentIter(node);

	wxutil::TreeModel::Row row = parentIter ? _model->AddItem(parentIter) : _model->AddItem();
	gtNode->getIter() = row.getItem();

	// Fill in the values
	row[_columns.node] = wxVariant(static_cast<void*>(node.get()));
	row[_columns.name] = node->name();

	row.SendItemAdded();

	// Insert this iterator into the node map to facilitate lookups
	std::pair<NodeMap::iterator, bool> result = _nodemap.insert(
		NodeMap::value_type(scene::INodeWeakPtr(node), gtNode)
	);

	// Return the GraphTreeNode reference
	return result.first->second;
}

void GraphTreeModel::erase(const scene::INodePtr& node)
{
	NodeMap::iterator found = _nodemap.find(scene::INodeWeakPtr(node));

	if (found != _nodemap.end())
	{
		// Remove this from the model...
		_model->RemoveItem(found->second->getIter());

		// ...and from our lookup table
		_nodemap.erase(found);
	}
}

const GraphTreeNodePtr& GraphTreeModel::find(const scene::INodePtr& node) const
{
	NodeMap::const_iterator found = _nodemap.find(scene::INodeWeakPtr(node));
	return (found != _nodemap.end()) ? found->second : _nullTreeNode;
}

void GraphTreeModel::clear()
{
	// Remove everything, wx plus nodemap
	_nodemap.clear();
	_model->Clear();
}

void GraphTreeModel::refresh()
{
#if defined(__linux__)
    _model->Clear();
#else
    // Create a new model from scratch and populate it
    _model = new wxutil::TreeModel(_columns);
#endif

	// Instantiate a scenegraph walker and visit every node in the graph
	// The walker also clears the graph in its constructor
	GraphTreeModelPopulator populator(*this, _visibleNodesOnly);
	GlobalSceneGraph().root()->traverse(populator);

    // Now sort the model once we have all nodes in the tree
    _model->SortModelByColumn(_columns.name);
}

void GraphTreeModel::setConsiderVisibleNodesOnly(bool visibleOnly)
{
	_visibleNodesOnly = visibleOnly;
}

void GraphTreeModel::updateSelectionStatus(const NotifySelectionUpdateFunc& notifySelectionChanged)
{
    // Don't traverse the entire scenegraph, visit selected nodes only
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        updateSelectionStatus(node, notifySelectionChanged);
    });
}

void GraphTreeModel::updateSelectionStatus(const scene::INodePtr& node,
										   const NotifySelectionUpdateFunc& notifySelectionChanged)
{
	NodeMap::const_iterator found = _nodemap.find(scene::INodeWeakPtr(node));

	GraphTreeNodePtr foundNode;

	if (found == _nodemap.end())
	{
		// The node is not in our map, it might have been previously hidden
		if (node->visible())
		{
			foundNode = insert(node);
		}
	}
	else
	{
		foundNode = found->second;
	}

	if (foundNode)
	{
		notifySelectionChanged(foundNode->getIter(), Node_isSelected(node));
	}
}

const GraphTreeNodePtr& GraphTreeModel::findParentNode(const scene::INodePtr& node) const
{
	scene::INodePtr parent = node->getParent();

	if (parent == NULL)
	{
		// No parent, return the NULL pointer
		return _nullTreeNode;
	}

	// Try to find the node
	NodeMap::const_iterator found = _nodemap.find(scene::INodeWeakPtr(parent));

	// Return NULL (empty shared_ptr) if not found
	return (found != _nodemap.end()) ? found->second : _nullTreeNode;
}

wxDataViewItem GraphTreeModel::findParentIter(const scene::INodePtr& node) const
{
	// Find the parent's GraphTreeNode
	const GraphTreeNodePtr& nodePtr = findParentNode(node);

	// Return an empty iterator if not found
	return (nodePtr != NULL) ? nodePtr->getIter() : wxDataViewItem();
}

const GraphTreeModel::TreeColumns& GraphTreeModel::getColumns() const
{
	return _columns;
}

wxutil::TreeModel::Ptr GraphTreeModel::getModel()
{
	return _model;
}

// Gets called when a new <instance> is inserted into the scenegraph
void GraphTreeModel::onSceneNodeInsert(const scene::INodePtr& node)
{
	insert(node); // wrap to the actual insert() method
}

// Gets called when <instance> is removed from the scenegraph
void GraphTreeModel::onSceneNodeErase(const scene::INodePtr& node)
{
	erase(node); // wrap to the actual erase() method
}

} // namespace ui
