#pragma once

#include <map>
#include <list>
#include <sigc++/signal.h>

#include "iscenegraph.h"
#include "imodule.h"
#include "ispacepartition.h"
#include <boost/enable_shared_from_this.hpp>

namespace scene
{

/**
 * Implementing class for the scenegraph.
 *
 * \ingroup scenegraph
 * \see scene::Graph
 */
class SceneGraph :
	public Graph,
	public boost::enable_shared_from_this<SceneGraph>
{
private:
	typedef std::list<Graph::Observer*> ObserverList;
	ObserverList _sceneObservers;

    sigc::signal<void> _sigBoundsChanged;

	// The root-element, the scenegraph starts here
	scene::INodePtr _root;

	// The space partitioning system
	ISpacePartitionSystemPtr _spacePartition;

	std::size_t _visitedSPNodes;
	std::size_t _skippedSPNodes;

public:
	SceneGraph();

	~SceneGraph();

	/** greebo: Adds/removes an observer from the scenegraph,
	 * 			to get notified upon insertions/deletions
	 */
	void addSceneObserver(Graph::Observer* observer);
	void removeSceneObserver(Graph::Observer* observer);

	// Triggers a call to all the connected Scene::Graph::Observers
	void sceneChanged();

	// Root node accessor methods
	const INodePtr& root() const;
	void setRoot(const INodePtr& newRoot);

	// greebo: Emits the "bounds changed" signal to all connected observers
	// Note: these are the WorkZone and the SelectionSystem, AFAIK
	void boundsChanged();

    /// Return the boundsChanged signal
    sigc::signal<void> signal_boundsChanged() const;

	void insert(const INodePtr& node);
	void erase(const INodePtr& node);

	void nodeBoundsChanged(const scene::INodePtr& node);

	// Walker variants
	void foreachNodeInVolume(const VolumeTest& volume, Walker& walker);
	void foreachVisibleNodeInVolume(const VolumeTest& volume, Walker& walker);

	// Lambda variants
	void foreachNode(const INode::VisitorFunc& functor);
	void foreachVisibleNode(const INode::VisitorFunc& functor);
	void foreachNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor);
	void foreachVisibleNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor);

	ISpacePartitionSystemPtr getSpacePartition();
private:
	void foreachNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor, bool visitHidden);

	// Recursive method used to descend the SpacePartition tree, returns FALSE if the walker signaled stop
	bool foreachNodeInVolume_r(const ISPNode& node, const VolumeTest& volume, 
							   const INode::VisitorFunc& functor, bool visitHidden);
};
typedef boost::shared_ptr<SceneGraph> SceneGraphPtr;

// Type used to register the GlobalSceneGraph in the module registry
class SceneGraphModule :
	public SceneGraph,
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<SceneGraphModule> SceneGraphModulePtr;

} // namespace scene
