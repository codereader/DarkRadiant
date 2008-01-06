#include "GraphTreeModel.h"

#include <gtk/gtk.h>
#include "scenelib.h"
#include <iostream>
#include "nameable.h"

#include "GraphTreeModelPopulator.h"
#include "GraphTreeModelSelectionUpdater.h"

namespace ui {

	namespace {
		inline NameablePtr Node_getNameable(const scene::INodePtr& node) {
			return boost::dynamic_pointer_cast<Nameable>(node);
		}
		
		inline std::string node_get_name(const scene::INodePtr& node) {
			NameablePtr nameable = Node_getNameable(node);
			return (nameable != NULL) ? nameable->name() : "node";
		}
	
		// Checks for NULL references and returns "" if node is NULL
		inline std::string node_get_name_safe(const scene::INodePtr& node) {
			return (node == NULL) ? "" : node_get_name(node);
		}
	}

GraphTreeModel::GraphTreeModel() :
	_model(gtk_tree_store_new(NUM_COLS, G_TYPE_POINTER, G_TYPE_STRING))
{
	// Subscribe to the scenegraph to get notified about insertions/deletions
	GlobalSceneGraph().addSceneObserver(this);
}

GraphTreeModel::~GraphTreeModel() {
	GlobalSceneGraph().removeSceneObserver(this);
	
	// Remove everything before shutting down
	clear();
}

const GraphTreeNodePtr& GraphTreeModel::insert(const scene::Instance& instance) {
	// Create a new GraphTreeNode
	GraphTreeNodePtr node(new GraphTreeNode(instance));
	
	// Insert this iterator below a possible parent iterator
	gtk_tree_store_insert(_model, node->getIter(), findParentIter(instance), 0);
	
	// Fill in the values
	gtk_tree_store_set(_model, node->getIter(), 
		COL_INSTANCE_POINTER, &instance,
		COL_NAME, getNodeCaption(instance.path().top()).c_str(),
		-1
	);
	
	// Insert this iterator into the node map to facilitate lookups
	std::pair<NodeMap::iterator, bool> result = _nodemap.insert(
		NodeMap::value_type(instance.path().top(), node)
	);
	
	// Return the GraphTreeNode reference
	return result.first->second;
}

void GraphTreeModel::erase(const scene::Instance& instance) {
	NodeMap::iterator found = _nodemap.find(instance.path().top());
	
	if (found != _nodemap.end()) {
		// Remove this from the GtkTreeStore...
		gtk_tree_store_remove(_model, found->second->getIter());
		
		// ...and from our lookup table
		_nodemap.erase(found);
	}
}

const GraphTreeNodePtr& GraphTreeModel::find(const scene::Instance& instance) const {
	NodeMap::const_iterator found = _nodemap.find(instance.path().top());
	return (found != _nodemap.end()) ? found->second : _nullTreeNode;
}

void GraphTreeModel::clear() {
	// Remove everything, GTK plus nodemap
	gtk_tree_store_clear(_model);
	_nodemap.clear();
}

void GraphTreeModel::refresh() {
	// Instantiate a scenegraph walker and visit every node in the graph
	// The walker also clears the graph in its constructor
	GraphTreeModelPopulator populator(*this);
	GlobalSceneGraph().traverse(populator);
}

void GraphTreeModel::updateSelectionStatus(GtkTreeSelection* selection) {
	GraphTreeModelSelectionUpdater updater(*this, selection);
	GlobalSceneGraph().traverse(updater);
}

void GraphTreeModel::updateSelectionStatus(GtkTreeSelection* selection, scene::Instance& instance) {
	NodeMap::const_iterator found = _nodemap.find(instance.path().top());
	
	if (found != _nodemap.end()) {
		if (Instance_isSelected(instance)) {

			// Select the row in the TreeView
			gtk_tree_selection_select_iter(selection, found->second->getIter());
		
			// Scroll to the row
			GtkTreeView* tv = gtk_tree_selection_get_tree_view(selection);
			GtkTreeModel* model = gtk_tree_view_get_model(tv);
			GtkTreePath* selectedPath = gtk_tree_model_get_path(
				model, found->second->getIter()
			);
			gtk_tree_view_expand_to_path(tv, selectedPath);
			gtk_tree_view_scroll_to_cell(tv, selectedPath, NULL, FALSE, 0, 0);
			gtk_tree_path_free(selectedPath);
		}
		else {
			gtk_tree_selection_unselect_iter(selection, found->second->getIter());
		}
	}
}

const GraphTreeNodePtr& GraphTreeModel::findParentNode(const scene::Instance& instance) const {
	const scene::Path& path = instance.path();
	
	if (path.size() <= 1) {
		// No parent, return the NULL pointer
		return _nullTreeNode;
	}
	
	// Try to find the node
	NodeMap::const_iterator found = _nodemap.find(path.parent());
	
	// Return NULL (empty shared_ptr) if not found
	return (found != _nodemap.end()) ? found->second : _nullTreeNode;
}

GtkTreeIter* GraphTreeModel::findParentIter(const scene::Instance& instance) const {
	// Find the parent's GraphTreeNode
	const GraphTreeNodePtr& nodePtr = findParentNode(instance);
	// Return a NULL nodeptr if not found
	return (nodePtr != NULL) ? nodePtr->getIter() : NULL;
}

std::string GraphTreeModel::getNodeCaption(const scene::INodePtr& node) {
	return node_get_name_safe(node);
}

GraphTreeModel::operator GtkTreeModel*() {
	return GTK_TREE_MODEL(_model);
}

// Gets called when a new <instance> is inserted into the scenegraph
void GraphTreeModel::onSceneNodeInsert(const scene::Instance& instance) {
	insert(instance); // wrap to the actual insert() method
}

// Gets called when <instance> is removed from the scenegraph
void GraphTreeModel::onSceneNodeErase(const scene::Instance& instance) {
	erase(instance); // wrap to the actual erase() method
}

} // namespace ui
