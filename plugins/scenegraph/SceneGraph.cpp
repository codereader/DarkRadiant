#include "SceneGraph.h"

#include "debugging/debugging.h"
#include "scene/InstanceWalkers.h"
#include "scenelib.h"

namespace scene
{

void SceneGraph::addSceneObserver(Graph::Observer* observer) {
	if (observer != NULL) {
		// Add the passed observer to the list
		_sceneObservers.push_back(observer);
	}
}

void SceneGraph::removeSceneObserver(Graph::Observer* observer) {
	// Cycle through the list of observers and call the moved method
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); ++i) {
		Graph::Observer* registered = *i;
		
		if (registered == observer) {
			_sceneObservers.erase(i);
			return; // Don't continue the loop, the iterator is obsolete 
		}
	}
}

void SceneGraph::sceneChanged() {
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); ++i) {
		Graph::Observer* observer = *i;
		observer->onSceneGraphChange();
	}
}

const INodePtr& SceneGraph::root() const
{
	return _root;
}
  
void SceneGraph::setRoot(const INodePtr& newRoot)
{
	if (_root != NULL)
	{
		// "Uninstantiate" the whole scene
		UninstanceSubgraphWalker walker;
		Node_traverseSubgraph(_root, walker);
	}

	_root = newRoot;

	if (_root != NULL)
	{
		// New root not NULL, "instantiate" the whole scene
		InstanceSubgraphWalker instanceWalker;
		Node_traverseSubgraph(_root, instanceWalker);
	}
}
  
void SceneGraph::boundsChanged()
{
    m_boundsChanged();
}

void SceneGraph::insert(const INodePtr& node)
{
    // Notify the graph tree model about the change
	sceneChanged();

	// Insert this node into our SP tree
	_spacePartition->link(node);

	// Call the onInsert event on the node
	node->onInsertIntoScene();
	
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); ++i) {
		(*i)->onSceneNodeInsert(node);
	}
}

void SceneGraph::erase(const INodePtr& node)
{
	_spacePartition->unLink(node);

	// Fire the onRemove event on the Node
	node->onRemoveFromScene();

	// Notify the graph tree model about the change
	sceneChanged();
	
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); ++i) {
		(*i)->onSceneNodeErase(node);
	}
}

SignalHandlerId SceneGraph::addBoundsChangedCallback(const SignalHandler& boundsChanged) {
    return m_boundsChanged.connectLast(boundsChanged);
}

void SceneGraph::removeBoundsChangedCallback(SignalHandlerId id) {
    m_boundsChanged.disconnect(id);
}

void SceneGraph::nodeBoundsChanged(const scene::INodePtr& node)
{
	_spacePartition->unLink(node);
	_spacePartition->link(node);
}

// RegisterableModule implementation
const std::string& SceneGraph::getName() const {
	static std::string _name(MODULE_SCENEGRAPH);
	return _name;
}

const StringSet& SceneGraph::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_SPACE_PARTITION_FACTORY);
	}

	return _dependencies;
}

void SceneGraph::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << "SceneGraph::initialiseModule called" << std::endl;

	_spacePartition = GlobalSpacePartitionSystemFactory().create();
}

} // namespace scene

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(scene::SceneGraphPtr(new scene::SceneGraph));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
