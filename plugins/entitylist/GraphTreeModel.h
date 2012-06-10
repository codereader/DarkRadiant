#pragma once

#include <boost/shared_ptr.hpp>
#include <map>
#include "iscenegraph.h"
#include "GraphTreeNode.h"

#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>

namespace ui
{

/**
 * greebo: This wraps around a GtkTreeModel which can be used
 *         in a GtkTreeView visualisation.
 *
 * The class provides basic routines to insert/remove scene::INodePtrs
 * into the model (the lookup should be performed fast).
 */
class GraphTreeModel :
	public scene::Graph::Observer
{
public:
	struct TreeColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		TreeColumns() { add(node); add(name); }

		Gtk::TreeModelColumn<scene::INode*> node;	// node ptr
		Gtk::TreeModelColumn<Glib::ustring> name;	// name
	};

private:
	// This maps scene::Nodes to TreeNode structures to allow fast lookups in the tree
	typedef std::map<scene::INodeWeakPtr, GraphTreeNodePtr> NodeMap;
	NodeMap _nodemap;

	// The NULL treenode, must always be empty
	const GraphTreeNodePtr _nullTreeNode;

	// The actual GTK model
	TreeColumns _columns;
	Glib::RefPtr<Gtk::TreeStore> _model;

	// The flag whether to skip invisible items
	bool _visibleNodesOnly;

public:
	GraphTreeModel();
	~GraphTreeModel();

	// Inserts the instance into the tree, returns the GtkTreeIter*
	const GraphTreeNodePtr& insert(const scene::INodePtr& node);
	// Removes the given instance from the tree
	void erase(const scene::INodePtr& node);

	// Tries to lookup the given node in the tree, can return the NULL node
	const GraphTreeNodePtr& find(const scene::INodePtr& node) const;

	// Remove everything from the TreeModel
	void clear();

	// Set whether invisible nodes should be considered, does NOT trigger a refresh!
	void setConsiderVisibleNodesOnly(bool visibleOnly);

	// Rebuilds the entire tree using a scene::Graph::Walker
	void refresh();

	// Updates the selection status of the entire tree
	void updateSelectionStatus(const Glib::RefPtr<Gtk::TreeSelection>& selection);

	// Updates the selection status of the given node only
	void updateSelectionStatus(const Glib::RefPtr<Gtk::TreeSelection>& selection, const scene::INodePtr& node);

	const TreeColumns& getColumns() const;
	Glib::RefPtr<Gtk::TreeModel> getModel();

	// Connects/disconnects this class as SceneObserver
	void connectToSceneGraph();
	void disconnectFromSceneGraph();

	// scene::Graph::Observer implementation

	// Gets called when a new <node> is inserted into the scenegraph
	void onSceneNodeInsert(const scene::INodePtr& node);
	// Gets called when <node> is removed from the scenegraph
	void onSceneNodeErase(const scene::INodePtr& node);

private:
	// Looks up the parent of the given node, can return NULL (empty shared_ptr)
	const GraphTreeNodePtr& findParentNode(const scene::INodePtr& node) const;

	// Tries to lookup the iterator to the parent item of the given node,
	// returns NULL if not found
	Gtk::TreeModel::iterator findParentIter(const scene::INodePtr& node) const;
};

} // namespace ui
