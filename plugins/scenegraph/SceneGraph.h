#pragma once

#include <map>
#include <list>
#include "signal/signal.h"
#include "scenelib.h"
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

	Signal _boundsChanged;

	// The root-element, the scenegraph starts here
	scene::INodePtr _root;

	// The space partitioning system
	ISpacePartitionSystemPtr _spacePartition;

	std::size_t _visitedSPNodes;
	std::size_t _skippedSPNodes;

public:
	SceneGraph();

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

	void insert(const INodePtr& node);
	void erase(const INodePtr& node);

	std::size_t addBoundsChangedCallback(const BoundsChangedFunc& callback);
	void removeBoundsChangedCallback(std::size_t handle);

	void nodeBoundsChanged(const scene::INodePtr& node);

	void foreachNodeInVolume(const VolumeTest& volume, Walker& walker);
	void foreachVisibleNodeInVolume(const VolumeTest& volume, Walker& walker);

	ISpacePartitionSystemPtr getSpacePartition();
private:
	// Recursive method used to descend the SpacePartition tree, returns FALSE if the walker signaled stop
	bool foreachNodeInVolume_r(const ISPNode& node, const VolumeTest& volume, Walker& walker, bool visitHidden);
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
