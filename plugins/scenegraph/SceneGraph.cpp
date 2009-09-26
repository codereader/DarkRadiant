#include "SceneGraph.h"

#include "debugging/debugging.h"
#include "scene/InstanceWalkers.h"
#include "scenelib.h"

void SceneGraph::addSceneObserver(scene::Graph::Observer* observer) {
	if (observer != NULL) {
		// Add the passed observer to the list
		_sceneObservers.push_back(observer);
	}
}

void SceneGraph::removeSceneObserver(scene::Graph::Observer* observer) {
	// Cycle through the list of observers and call the moved method
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); ++i) {
		scene::Graph::Observer* registered = *i;
		
		if (registered == observer) {
			_sceneObservers.erase(i);
			return; // Don't continue the loop, the iterator is obsolete 
		}
	}
}

void SceneGraph::sceneChanged() {
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); ++i) {
		scene::Graph::Observer* observer = *i;
		observer->onSceneGraphChange();
	}
}

scene::INodePtr SceneGraph::root() {
	ASSERT_MESSAGE(_root != NULL, "scenegraph root does not exist");
	return _root;
}
  
void SceneGraph::insert_root(scene::INodePtr root) {
	ASSERT_MESSAGE(_root == NULL, "scenegraph root already exists");

	scene::Path path;
	scene::InstanceSubgraphWalker instanceWalker(path);
    Node_traverseSubgraph(root, instanceWalker);

	_root = root;
}
  
void SceneGraph::erase_root() {
    ASSERT_MESSAGE(_root != NULL, "scenegraph root does not exist");

    scene::Path path;
	scene::UninstanceSubgraphWalker walker(path);
    Node_traverseSubgraph(_root, walker);

	_root = scene::INodePtr();
}

void SceneGraph::boundsChanged() {
    m_boundsChanged();
}

void SceneGraph::insert(const scene::INodePtr& node) {
    // Notify the graph tree model about the change
	sceneChanged();

	// Call the onInsert event on the node
	node->onInsertIntoScene();
	
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); ++i) {
		(*i)->onSceneNodeInsert(node);
	}
}

void SceneGraph::erase(const scene::INodePtr& node) {

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

// RegisterableModule implementation
const std::string& SceneGraph::getName() const {
	static std::string _name(MODULE_SCENEGRAPH);
	return _name;
}

const StringSet& SceneGraph::getDependencies() const {
	static StringSet _dependencies; // no dependencies
	return _dependencies;
}

void SceneGraph::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "SceneGraph::initialiseModule called\n";
}

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(SceneGraphPtr(new SceneGraph));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
