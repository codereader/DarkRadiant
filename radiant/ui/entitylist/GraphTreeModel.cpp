#include "GraphTreeModel.h"

#include "iselectable.h"
#include "iselection.h"

#include "GraphTreeModelPopulator.h"
#include "debugging/ScenegraphUtils.h"

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

const GraphTreeNode::Ptr& GraphTreeModel::insert(const scene::INodePtr& node)
{
	// Insert this iterator below a possible parent iterator
	auto parentIter = findParentIter(node);

	wxutil::TreeModel::Row row = parentIter ? _model->AddItemUnderParent(parentIter) : _model->AddItem();

	// Create a new GraphTreeNode
	auto gtNode = std::make_shared<GraphTreeNode>(node, row.getItem());

    // Assign root node member
    if (node->getNodeType() == scene::INode::Type::MapRoot)
    {
        _mapRootNode = gtNode;
    }

	// Fill in the values
	row[_columns.node] = wxVariant(node.get());
	row[_columns.name] = node->name();

	row.SendItemAdded();

	// Insert this iterator into the node map to facilitate lookups
	// Return the GraphTreeNode reference
	return _nodemap.emplace(node, gtNode).first->second;
}

void GraphTreeModel::erase(const scene::INodePtr& node)
{
	auto found = _nodemap.find(node);

	if (found != _nodemap.end())
	{
		// Remove this from the model...
		_model->RemoveItem(found->second->getIter());

		// ...and from our lookup table
		_nodemap.erase(found);

        if (_mapRootNode && _mapRootNode->getNode() == node)
        {
            _mapRootNode.reset();
        }
	}
}

const GraphTreeNode::Ptr& GraphTreeModel::find(const scene::INodePtr& node) const
{
	auto found = _nodemap.find(node);
	return found != _nodemap.end() ? found->second : _nullTreeNode;
}

void GraphTreeModel::clear()
{
	// Remove everything, wx plus nodemap
	_nodemap.clear();
	_model->Clear();
    _mapRootNode.reset();
}

void GraphTreeModel::refresh()
{
#if defined(__linux__)
    _model->Clear();
#else
    // Create a new model from scratch and populate it
    _model = new wxutil::TreeModel(_columns);
#endif

    if (!GlobalSceneGraph().root()) return;

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
    if (auto found = _nodemap.find(node); found != _nodemap.end())
    {
        notifySelectionChanged(found->second->getIter(), Node_isSelected(node));
    }
}

wxDataViewItem GraphTreeModel::findParentIter(const scene::INodePtr& node)
{
    switch (node->getNodeType())
    {
    case scene::INode::Type::MapRoot:
        return wxDataViewItem(); // root node is inserted at the root level

    case scene::INode::Type::Entity:
        if (!_mapRootNode)
        {
            // Create a new map root node right here
            insert(node->getParent());
        }
        
        return _mapRootNode->getIter();

    default:
        rWarning() << "Node type " << getNameForNodeType(node->getNodeType()) << " should not be inserted into the tree" << std::endl;
        return wxDataViewItem();
    }
}

const GraphTreeModel::TreeColumns& GraphTreeModel::getColumns() const
{
	return _columns;
}

wxutil::TreeModel::Ptr GraphTreeModel::getModel()
{
	return _model;
}

void GraphTreeModel::onSceneNodeInsert(const scene::INodePtr& node)
{
    if (!NodeIsRelevant(node)) return;

    insert(node); // wrap to the actual insert() method
}

void GraphTreeModel::onSceneNodeErase(const scene::INodePtr& node)
{
    if (!NodeIsRelevant(node)) return;

    erase(node); // wrap to the actual erase() method
}

} // namespace
