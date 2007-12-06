#include "GraphTreeModel.h"

#include <gtk/gtk.h>
#include "scenelib.h"
#include <iostream>
#include "nameable.h"

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

void GraphTreeModel::insert(const scene::Instance& instance) {
	// Allocate a new GtkTreeIter and acquire a new tree row from GTK
	GtkTreeIterPtr iter(new GtkTreeIter);
	
	// Insert this iterator below a possible parent iterator
	gtk_tree_store_insert(_model, iter.get(), findParent(instance).get(), 0);
	
	// Fill in the values
	gtk_tree_store_set(_model, iter.get(), 
		COL_INSTANCE_POINTER, &instance,
		COL_NAME, node_get_name_safe(instance.path().top()).c_str(),
		-1
	);
	
	// Insert this iterator into the node map to facilitate lookups
	_nodemap.insert(NodeMap::value_type(instance.path().top(), iter));
}

void GraphTreeModel::erase(const scene::Instance& instance) {
	NodeMap::iterator found = _nodemap.find(instance.path().top());
	
	if (found != _nodemap.end()) {
		// Remove this from the GtkTreeStore...
		gtk_tree_store_remove(_model, found->second.get());
		
		// ...and from our lookup table
		_nodemap.erase(found);
	}
}

const GraphTreeModel::GtkTreeIterPtr& GraphTreeModel::findParent(const scene::Instance& instance) const {
	const scene::Path& path = instance.path();
	
	if (path.size() <= 1) {
		// No parent, return the NULL pointer
		return _nullGtkTreeIter;
	}
	
	// Try to find the node
	NodeMap::const_iterator found = _nodemap.find(path.parent());
	
	// Return NULL (empty shared_ptr) if not found
	return (found != _nodemap.end()) ? found->second : _nullGtkTreeIter;
}

GraphTreeModel::operator GtkTreeModel*() {
	return GTK_TREE_MODEL(_model);
}

} // namespace ui
