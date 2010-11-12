#include "GraphTreeModel.h"

#include "scenelib.h"
#include <iostream>
#include "nameable.h"

#include "GraphTreeModelPopulator.h"
#include "GraphTreeModelSelectionUpdater.h"

#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treeview.h>

namespace ui {

	namespace {
		inline NameablePtr Node_getNameable(const scene::INodePtr& node)
		{
			return boost::dynamic_pointer_cast<Nameable>(node);
		}

		inline std::string node_get_name(const scene::INodePtr& node)
		{
			NameablePtr nameable = Node_getNameable(node);
			return (nameable != NULL) ? nameable->name() : "node";
		}

		// Checks for NULL references and returns "" if node is NULL
		inline std::string node_get_name_safe(const scene::INodePtr& node)
		{
			return (node == NULL) ? "" : node_get_name(node);
		}
	}

GraphTreeModel::GraphTreeModel() :
	_model(Gtk::TreeStore::create(_columns))
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
	row[_columns.name] = getNodeCaption(node);

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
	GraphTreeModelPopulator populator(*this);
	Node_traverseSubgraph(GlobalSceneGraph().root(), populator);
}

void GraphTreeModel::updateSelectionStatus(const Glib::RefPtr<Gtk::TreeSelection>& selection)
{
	GraphTreeModelSelectionUpdater updater(*this, selection);
	Node_traverseSubgraph(GlobalSceneGraph().root(), updater);
}

void GraphTreeModel::updateSelectionStatus(const Glib::RefPtr<Gtk::TreeSelection>& selection, const scene::INodePtr& node)
{
	NodeMap::const_iterator found = _nodemap.find(scene::INodeWeakPtr(node));

	if (found != _nodemap.end())
	{
		if (Node_isSelected(node))
		{
			// Select the row in the TreeView
			selection->select(found->second->getIter());

			// Scroll to the row
			Gtk::TreeView* tv = selection->get_tree_view();

			Gtk::TreeModel::Path selectedPath(found->second->getIter());

			tv->expand_to_path(selectedPath);
			tv->scroll_to_row(selectedPath, 0.3f);
		}
		else
		{
			selection->unselect(found->second->getIter());
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

std::string GraphTreeModel::getNodeCaption(const scene::INodePtr& node)
{
	return node_get_name_safe(node);
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
