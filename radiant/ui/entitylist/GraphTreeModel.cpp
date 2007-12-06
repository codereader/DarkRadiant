#include "GraphTreeModel.h"

#include <gtk/gtk.h>
#include "scenelib.h"
#include <iostream>
#include "nameable.h"
#include "GraphTreeModelPopulator.h"

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
{}

GraphTreeModel::~GraphTreeModel() {
	// Remove everything before shutting down
	clear();
}

void GraphTreeModel::insert(const scene::Instance& instance) {
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
	_nodemap.insert(NodeMap::value_type(instance.path().top(), node));
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

} // namespace ui
