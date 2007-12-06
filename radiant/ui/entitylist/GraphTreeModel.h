#ifndef GRAPHTREEMODEL_H_
#define GRAPHTREEMODEL_H_

#include <gtk/gtktreestore.h>
#include <boost/shared_ptr.hpp>
#include <map>
#include "iscenegraph.h"
#include "GraphTreeNode.h"

namespace ui {

/**
 * greebo: This wraps around a GtkTreeModel which can be used
 *         in a GtkTreeView visualisation.
 * 
 * The class provides basic routines to insert/remove scene::Instances
 * into the model (the lookup should be performed fast).
 */
class GraphTreeModel
{
public:
	// The enumeration of GTK column names
	enum {
		COL_INSTANCE_POINTER,
		COL_NAME,
		NUM_COLS
	};

private:
	// This maps scene::Nodes to TreeNode structures to allow fast lookups in the tree
	typedef std::map<scene::INodePtr, GraphTreeNodePtr> NodeMap;
	NodeMap _nodemap;
	
	// The NULL treenode, must always be empty
	const GraphTreeNodePtr _nullTreeNode;
	
	// The actual GTK model
	GtkTreeStore* _model;
public:
	GraphTreeModel();
	
	~GraphTreeModel();
	
	// Inserts the instance into the tree
	void insert(const scene::Instance& instance);
	// Removes the given instance from the tree
	void erase(const scene::Instance& instance);
	
	// Remove everything from the TreeModel
	void clear();
	
	// Rebuilds the entire tree using a scene::Graph::Walker
	void refresh();
	
	// Operator-cast to GtkTreeModel to allow for implicit conversion
	operator GtkTreeModel*();
	
private:
	// Looks up the parent of the given instance, can return NULL (empty shared_ptr)
	const GraphTreeNodePtr& findParentNode(const scene::Instance& instance) const;
	
	// Tries to lookup the iterator to the parent item of the given instance, 
	// returns NULL if not found
	GtkTreeIter* findParentIter(const scene::Instance& instance) const;
	
	// Get the caption string used to display the node in the tree
	std::string getNodeCaption(const scene::INodePtr& node);
};

} // namespace ui

#endif /*GRAPHTREEMODEL_H_*/
