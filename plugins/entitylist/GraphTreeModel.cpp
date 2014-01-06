#include "GraphTreeModel.h"

#include <iostream>
#include "iselectable.h"

#include "GraphTreeModelPopulator.h"
#include "GraphTreeModelSelectionUpdater.h"

#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treeview.h>

namespace ui
{

GraphTreeModel::GraphTreeModel() :
	_model(Gtk::TreeStore::create(_columns)),
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
	Gtk::TreeModel::iterator parentIter = findParentIter(node);

	gtNode->getIter() = parentIter ? _model->append(parentIter->children()) : _model->append();

	// Fill in the values
	Gtk::TreeModel::Row row = *gtNode->getIter();

	row[_columns.node] = node.get();
	row[_columns.name] = node->name();

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
		// Remove this from the GtkTreeStore...
		_model->erase(found->second->getIter());

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
	// Remove everything, GTK plus nodemap
	_nodemap.clear();
	_model->clear();
}

void GraphTreeModel::refresh()
{
	// Instantiate a scenegraph walker and visit every node in the graph
	// The walker also clears the graph in its constructor
	GraphTreeModelPopulator populator(*this, _visibleNodesOnly);
	GlobalSceneGraph().root()->traverse(populator);
}

void GraphTreeModel::setConsiderVisibleNodesOnly(bool visibleOnly)
{
	_visibleNodesOnly = visibleOnly;
}

void GraphTreeModel::updateSelectionStatus(const Glib::RefPtr<Gtk::TreeSelection>& selection)
{
	GraphTreeModelSelectionUpdater updater(*this, selection);
	GlobalSceneGraph().root()->traverse(updater);
}

void GraphTreeModel::updateSelectionStatus(const Glib::RefPtr<Gtk::TreeSelection>& selection, const scene::INodePtr& node)
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
		if (Node_isSelected(node))
		{
			// Select the row in the TreeView
			selection->select(foundNode->getIter());

			// Scroll to the row
			Gtk::TreeView* tv = selection->get_tree_view();

			Gtk::TreeModel::Path selectedPath(foundNode->getIter());

			tv->expand_to_path(selectedPath);
			tv->scroll_to_row(selectedPath, 0.3f);
		}
		else
		{
			selection->unselect(foundNode->getIter());
		}
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

Gtk::TreeModel::iterator GraphTreeModel::findParentIter(const scene::INodePtr& node) const
{
	// Find the parent's GraphTreeNode
	const GraphTreeNodePtr& nodePtr = findParentNode(node);

	// Return an empty iterator if not found
	return (nodePtr != NULL) ? nodePtr->getIter() : Gtk::TreeModel::iterator();
}

const GraphTreeModel::TreeColumns& GraphTreeModel::getColumns() const
{
	return _columns;
}

Glib::RefPtr<Gtk::TreeModel> GraphTreeModel::getModel()
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
