#pragma once

#include <map>
#include <list>
#include <sigc++/signal.h>
#include <sigc++/connection.h>

#include "iscenegraph.h"
#include "imodule.h"
#include "ispacepartition.h"
#include "imap.h"
#include "iundo.h"

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
	public std::enable_shared_from_this<SceneGraph>
{
private:
	typedef std::list<Graph::Observer*> ObserverList;
	ObserverList _sceneObservers;

    sigc::signal<void> _sigBoundsChanged;

	// The root-element, the scenegraph starts here
    IMapRootNodePtr _root;

	// The space partitioning system
	ISpacePartitionSystemPtr _spacePartition;

	std::size_t _visitedSPNodes;
	std::size_t _skippedSPNodes;

    // During partition traversal all link/unlink calls are buffered and
    // performed later on.
    enum ActionType
    {
        Insert,
        Erase,
        BoundsChange,
    };
    typedef std::pair<ActionType, scene::INodePtr> NodeAction;
    typedef std::list<NodeAction> BufferedActions;
    BufferedActions _actionBuffer;

    bool _traversalOngoing;

    sigc::connection _undoEventHandler;

public:
	SceneGraph();

	~SceneGraph();

	/** greebo: Adds/removes an observer from the scenegraph,
	 * 			to get notified upon insertions/deletions
	 */
	void addSceneObserver(Graph::Observer* observer) override;
    void removeSceneObserver(Graph::Observer* observer) override;

	// Triggers a call to all the connected Scene::Graph::Observers
    void sceneChanged() override;

	// Root node accessor methods
    const IMapRootNodePtr& root() const override;
    void setRoot(const IMapRootNodePtr& newRoot) override;

	// greebo: Emits the "bounds changed" signal to all connected observers
	// Note: these are the WorkZone and the SelectionSystem, AFAIK
    void boundsChanged() override;

    /// Return the boundsChanged signal
    sigc::signal<void> signal_boundsChanged() const override;

    void insert(const INodePtr& node) override;
    void erase(const INodePtr& node) override;

    void nodeBoundsChanged(const scene::INodePtr& node) override;

	// Walker variants
    void foreachNodeInVolume(const VolumeTest& volume, Walker& walker) override;
    void foreachVisibleNodeInVolume(const VolumeTest& volume, Walker& walker) override;

	// Lambda variants
    void foreachNode(const INode::VisitorFunc& functor) override;
    void foreachVisibleNode(const INode::VisitorFunc& functor) override;
    void foreachNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor) override;
    void foreachVisibleNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor) override;

    ISpacePartitionSystemPtr getSpacePartition() override;
private:
	void foreachNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor, bool visitHidden);

	// Recursive method used to descend the SpacePartition tree, returns FALSE if the walker signaled stop
	bool foreachNodeInVolume_r(const ISPNode& node, const VolumeTest& volume, 
							   const INode::VisitorFunc& functor, bool visitHidden);

    void flushActionBuffer();

    void onUndoEvent(IUndoSystem::EventType type, const std::string& operationName);
};
typedef std::shared_ptr<SceneGraph> SceneGraphPtr;

// Type used to register the GlobalSceneGraph in the module registry
class SceneGraphModule :
	public SceneGraph,
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const IApplicationContext& ctx);
};
typedef std::shared_ptr<SceneGraphModule> SceneGraphModulePtr;

} // namespace scene
