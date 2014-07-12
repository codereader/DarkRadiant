#pragma once

#include <boost/shared_ptr.hpp>
#include <map>
#include "iscenegraph.h"
#include "GraphTreeNode.h"

#include "wxutil/TreeModel.h"

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
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			node(add(wxutil::TreeModel::Column::Integer)),
			name(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column node;	// node ptr
		wxutil::TreeModel::Column name;	// name
	};

private:
	// This maps scene::Nodes to TreeNode structures to allow fast lookups in the tree
	typedef std::map<scene::INodeWeakPtr, GraphTreeNodePtr> NodeMap;
	NodeMap _nodemap;

	// The NULL treenode, must always be empty
	const GraphTreeNodePtr _nullTreeNode;

	// The actual model
	TreeColumns _columns;
	wxutil::TreeModel* _model;

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

	typedef std::function<void (const wxDataViewItem&, bool)> NotifySelectionUpdateFunc;

	// Updates the selection status of the entire tree
	void updateSelectionStatus(const NotifySelectionUpdateFunc& notifySelectionChanged);

	// Updates the selection status of the given node only
	void updateSelectionStatus(const scene::INodePtr& node,
		const NotifySelectionUpdateFunc& notifySelectionChanged);

	const TreeColumns& getColumns() const;
	wxutil::TreeModel* getModel();

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
	wxDataViewItem findParentIter(const scene::INodePtr& node) const;
};

} // namespace ui
