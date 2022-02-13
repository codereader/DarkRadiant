#include "SceneGraph.h"

#include "ivolumetest.h"
#include "itextstream.h"

#include "scene/InstanceWalkers.h"
#include "debugging/debugging.h"

#include "math/AABB.h"
#include "Octree.h"
#include "SceneGraphFactory.h"
#include "util/ScopedBoolLock.h"
#include "module/StaticModule.h"

namespace scene
{

SceneGraph::SceneGraph() :
	_spacePartition(new Octree),
	_visitedSPNodes(0),
	_skippedSPNodes(0),
    _traversalOngoing(false)
{}

SceneGraph::~SceneGraph()
{
	// Make sure the scene graph is properly uninstantiated
	if (root())
	{
        flushActionBuffer();
		setRoot(IMapRootNodePtr());
	}
}

void SceneGraph::addSceneObserver(Graph::Observer* observer)
{
	if (observer != nullptr)
    {
		// Add the passed observer to the list
		_sceneObservers.push_back(observer);
	}
}

void SceneGraph::removeSceneObserver(Graph::Observer* observer)
{
	// Cycle through the list of observers and call the moved method
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); ++i)
    {
		Graph::Observer* registered = *i;

		if (registered == observer)
        {
			_sceneObservers.erase(i);
			return; // Don't continue the loop, the iterator is obsolete
		}
	}
}

void SceneGraph::sceneChanged()
{
    for (Graph::Observer* observer : _sceneObservers)
    {
		observer->onSceneGraphChange();
	}
}

const IMapRootNodePtr& SceneGraph::root() const
{
	return _root;
}

void SceneGraph::setRoot(const IMapRootNodePtr& newRoot)
{
	if (_root == newRoot)
	{
		return;
	}

    _undoEventHandler.disconnect();

	if (_root)
	{
		// "Uninstantiate" the whole scene
		UninstanceSubgraphWalker walker(*this);
		_root->traverse(walker);
	}

	_root = newRoot;

	// Refresh the space partition class
	_spacePartition = std::make_shared<Octree>();

	if (_root)
	{
		// New root not NULL, "instantiate" the whole scene
		GraphPtr self = shared_from_this();
		InstanceSubgraphWalker instanceWalker(self);
		_root->traverse(instanceWalker);

        _undoEventHandler = _root->getUndoSystem().signal_undoEvent().connect(
            sigc::mem_fun(this, &SceneGraph::onUndoEvent)
        );
	}
}

void SceneGraph::onUndoEvent(IUndoSystem::EventType type, const std::string& operationName)
{
    if (type == IUndoSystem::EventType::OperationUndone)
    {
        // Trigger the onPostUndo event on all scene nodes
        foreachNode([&](const INodePtr& node)->bool
        {
            node->onPostUndo();
            return true;
        });

        sceneChanged();
    }
    else if (type == IUndoSystem::EventType::OperationRedone)
    {
        // Trigger the onPostRedo event on all scene nodes
        foreachNode([&](const INodePtr& node)->bool
        {
            node->onPostRedo();
            return true;
        });

        sceneChanged();
    }
}

void SceneGraph::boundsChanged()
{
    _sigBoundsChanged();
}

sigc::signal<void> SceneGraph::signal_boundsChanged() const
{
    return _sigBoundsChanged;
}

void SceneGraph::insert(const INodePtr& node)
{
    if (_traversalOngoing)
    {
        _actionBuffer.push_back(NodeAction(Insert, node));
        return;
    }

    // Notify the graph tree model about the change
	sceneChanged();

	// Insert this node into our SP tree
	_spacePartition->link(node);

	// Call the onInsert event on the node
    assert(_root);
	node->onInsertIntoScene(*_root);

	for (auto i : _sceneObservers)
    {
		i->onSceneNodeInsert(node);
	}
}

void SceneGraph::erase(const INodePtr& node)
{
    if (_traversalOngoing)
    {
        _actionBuffer.push_back(NodeAction(Erase, node));
        return;
    }

	_spacePartition->unlink(node);

	// Fire the onRemove event on the Node
    assert(_root);
    node->onRemoveFromScene(*_root);

	// Notify the graph tree model about the change
	sceneChanged();

    for (auto i : _sceneObservers)
    {
		i->onSceneNodeErase(node);
	}
}

void SceneGraph::nodeBoundsChanged(const INodePtr& node)
{
    if (_traversalOngoing)
    {
        _actionBuffer.push_back(NodeAction(BoundsChange, node));
        return;
    }

	if (_spacePartition->unlink(node))
	{
		// unlink returned true, so the given node was linked before => re-link it
		_spacePartition->link(node);
	}
}

void SceneGraph::foreachNode(const INode::VisitorFunc& functor)
{
	if (!_root) return;

	// First hit the root node
	if (!functor(_root))
	{
		return;
	}

	_root->foreachNode(functor);
}

void SceneGraph::foreachVisibleNode(const INode::VisitorFunc& functor)
{
	// Walk the scene using a small adaptor excluding hidden nodes
	foreachNode([&] (const INodePtr& node)->bool
	{
		// On visible nodes, invoke the functor
		if (node->visible())
		{
			return functor(node);
		}

		// On hidden nodes, just return true to continue traversal
		return true;
	});
}

void SceneGraph::foreachNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor)
{
	foreachNodeInVolume(volume, functor, true); // visit hidden
}

void SceneGraph::foreachVisibleNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor)
{
	foreachNodeInVolume(volume, functor, false); // don't visit hidden
}

void SceneGraph::foreachNodeInVolume(const VolumeTest& volume, const INode::VisitorFunc& functor, bool visitHidden)
{
    // Acquire the worldAABB() of the scenegraph root - if any node got changed in the graph
    // the scenegraph's root bounds are marked as "dirty" and the bounds will be re-calculated
    // which in turn might trigger a re-link in the Octree. We want to avoid that the Octree
    // changes during traversal so let's call this now. If nothing got changed, this call is very cheap.
    if (_root != nullptr) _root->worldAABB();

    {
        // Buffer any calls that might happen in between
        util::ScopedBoolLock traversal(_traversalOngoing);

        // Descend the SpacePartition tree and call the walker for each (partially) visible member
        ISPNodePtr root = _spacePartition->getRoot();

        _visitedSPNodes = _skippedSPNodes = 0;

        foreachNodeInVolume_r(*root, volume, functor, visitHidden);

        _visitedSPNodes = _skippedSPNodes = 0;
    }

    // Traversal finished, flush the action buffer
    flushActionBuffer();
}

void SceneGraph::foreachNodeInVolume(const VolumeTest& volume, Walker& walker)
{
	// Use a small adaptor lambda to dispatch calls to the walker
	foreachNodeInVolume(volume,
		[&] (const INodePtr& node) { return walker.visit(node); },
		true); // visit hidden
}

void SceneGraph::foreachVisibleNodeInVolume(const VolumeTest& volume, Walker& walker)
{
	// Use a small adaptor lambda to dispatch calls to the walker
	foreachNodeInVolume(volume,
		[&] (const INodePtr& node) { return walker.visit(node); },
		false); // don't visit hidden
}

bool SceneGraph::foreachNodeInVolume_r(const ISPNode& node, const VolumeTest& volume,
									   const INode::VisitorFunc& functor, bool visitHidden)
{
	_visitedSPNodes++;

	// Visit all members
	const ISPNode::MemberList& members = node.getMembers();

	for (ISPNode::MemberList::const_iterator m = members.begin();
		 m != members.end(); /* in-loop increment */)
	{
		// Skip hidden nodes, if specified
		if (!visitHidden && !(*m)->visible())
		{
			++m;
			continue;
		}

		// We're done, as soon as the walker returns FALSE
		if (!functor(*m++))
		{
			return false;
		}
	}

	// Now consider the children
	const ISPNode::NodeList& children = node.getChildNodes();

	for (ISPNode::NodeList::const_iterator i = children.begin(); i != children.end(); ++i)
	{
		if (volume.TestAABB((*i)->getBounds()) == VOLUME_OUTSIDE)
		{
			// Skip this node, not visible
			_skippedSPNodes++;
			continue;
		}

		// Traverse all the children too, enter recursion
		if (!foreachNodeInVolume_r(**i, volume, functor, visitHidden))
		{
			// The walker returned false somewhere in the recursion depths, propagate this message
			return false;
		}
	}

	return true; // continue traversal
}

ISpacePartitionSystemPtr SceneGraph::getSpacePartition()
{
	return _spacePartition;
}

void SceneGraph::flushActionBuffer()
{
    // Do any actions now, in the same order they came in
    for (NodeAction& action : _actionBuffer)
    {
        switch (action.first)
        {
        case Insert:
            insert(action.second);
            break;
        case Erase:
            erase(action.second);
            break;
        case BoundsChange:
            nodeBoundsChanged(action.second);
            break;
        };
    }

    _actionBuffer.clear();
}

// RegisterableModule implementation
const std::string& SceneGraphModule::getName() const
{
	static std::string _name(MODULE_SCENEGRAPH);
	return _name;
}

const StringSet& SceneGraphModule::getDependencies() const
{
	static StringSet _dependencies; // no deps
	return _dependencies;
}

void SceneGraphModule::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called" << std::endl;
}

// Static module instances
module::StaticModuleRegistration<SceneGraphModule> sceneGraphModule;
module::StaticModuleRegistration<SceneGraphFactory> sceneGraphFactory;

} // namespace scene
