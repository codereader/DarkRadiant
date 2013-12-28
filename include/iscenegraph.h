#pragma once

#include <cstddef>
#include "imodule.h"
#include "inode.h"
#include "ipath.h"
#include <boost/weak_ptr.hpp>
#include <sigc++/signal.h>

/**
 * \defgroup scenegraph Scenegraph
 *
 * \namespace scene
 * \ingroup scenegraph
 * Interfaces and types relating to the scene-graph.
 */

// String identifier for the registry module
const std::string MODULE_SCENEGRAPH("SceneGraph");

class VolumeTest;

namespace scene
{

class ISpacePartitionSystem;
typedef boost::shared_ptr<ISpacePartitionSystem> ISpacePartitionSystemPtr;

/**
* A scene-graph - a Directed Acyclic Graph (DAG).
*
* Each node may refer to zero or more 'child' nodes (directed).
* A node may never have itself as one of its ancestors (acyclic).
*/
class Graph
{
public:

	/* greebo: Derive from this class to get notified on scene changes
	 */
	class Observer
	{
	public:
		// destructor
		virtual ~Observer() {}

		// Gets called when anything in the scenegraph changes
		virtual void onSceneGraphChange() {}

		// Gets called when a new <node> is inserted into the scenegraph
		virtual void onSceneNodeInsert(const INodePtr& node) {}

		// Gets called when <node> is removed from the scenegraph
		virtual void onSceneNodeErase(const INodePtr& node) {}
	};

	// Returns the root-node of the graph.
	virtual const INodePtr& root() const = 0;

	// Sets the root-node of the graph to be 'newRoot'.
	virtual void setRoot(const INodePtr& newRoot) = 0;

	// greebo: Adds a node to the scenegraph
	virtual void insert(const scene::INodePtr& node) = 0;
	// Removes a node from the scenegraph
	virtual void erase(const scene::INodePtr& node) = 0;

	/// \brief Invokes all scene-changed callbacks. Called when any part of the scene changes the way it will appear when the scene is rendered.
	/// \todo Move to a separate class.
	virtual void sceneChanged() = 0;
	/// \brief Add a \p callback to be invoked when the scene changes.
	/// \todo Move to a separate class.
	virtual void addSceneObserver(Observer* observer) = 0;
	// greebo: Remove the scene observer from the list
	virtual void removeSceneObserver(Observer* observer) = 0;

    /// Accessor for the signal emitted when bounds are changed
    virtual sigc::signal<void> signal_boundsChanged() const = 0;

	/// \brief Invokes all bounds-changed callbacks. Called when the bounds of any instance in the scene change.
	/// \todo Move to a separate class.
	virtual void boundsChanged() = 0;

	// A specific node has changed its bounds
	virtual void nodeBoundsChanged(const scene::INodePtr& node) = 0;

	// A walker class to be used in "foreachNodeInVolume"
	class Walker
	{
	public:
		virtual ~Walker() {}

		// Called for each visited node, returns TRUE if traversal should continue
		virtual bool visit(const INodePtr& node) = 0;
	};

	// Visit each scene node in the given volume using the given walker class, even hidden ones
	virtual void foreachNodeInVolume(const VolumeTest& volume, Walker& walker) = 0;

	// Same as above, but culls any hidden nodes
	virtual void foreachVisibleNodeInVolume(const VolumeTest& volume, Walker& walker) = 0;

	// Call the functor on each scene node in the entire scenegraph, including hidden ones
	virtual void foreachNode(const INode::VisitorFunc& functor) = 0;

	// Call the functor on each scene node in the entire scenegraph, excluding hidden ones
	virtual void foreachVisibleNode(const INode::VisitorFunc& functor) = 0;

	// Call the functor on each scene node in the given volume, even hidden ones
	virtual void foreachNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor) = 0;

	// Same as above, but culls any hidden nodes
	virtual void foreachVisibleNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor) = 0;

	// Returns the associated spacepartition
	virtual ISpacePartitionSystemPtr getSpacePartition() = 0;
};
typedef boost::shared_ptr<Graph> GraphPtr;
typedef boost::weak_ptr<Graph> GraphWeakPtr;

class Cloneable
{
public:
	/// \brief destructor
	virtual ~Cloneable() {}

	/// \brief Returns a copy of itself.
	virtual scene::INodePtr clone() const = 0;
};
typedef boost::shared_ptr<Cloneable> CloneablePtr;

} // namespace

// Accessor to the singleton scenegraph, used for the main map
inline scene::Graph& GlobalSceneGraph()
{
	// Cache the reference locally
	static scene::Graph& _sceneGraph(
		*boost::dynamic_pointer_cast<scene::Graph>(
			module::GlobalModuleRegistry().getModule(MODULE_SCENEGRAPH)
		)
	);
	return _sceneGraph;
}

inline void SceneChangeNotify()
{
	GlobalSceneGraph().sceneChanged();
}
