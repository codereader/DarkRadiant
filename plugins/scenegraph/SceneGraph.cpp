#include "SceneGraph.h"

#include "debugging/debugging.h"
#include "scenelib.h"
#include "instancelib.h"

void SceneGraph::addSceneObserver(scene::Graph::Observer* observer) {
	if (observer != NULL) {
		// Add the passed observer to the list
		_sceneObservers.push_back(observer);
	}
}

void SceneGraph::removeSceneObserver(scene::Graph::Observer* observer) {
	// Cycle through the list of observers and call the moved method
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); i++) {
		scene::Graph::Observer* registered = *i;
		
		if (registered == observer) {
			_sceneObservers.erase(i++);
			return; // Don't continue the loop, the iterator is obsolete 
		}
	}
}

void SceneGraph::sceneChanged() {
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); i++) {
		scene::Graph::Observer* observer = *i;
		observer->onSceneGraphChange();
	}
}

scene::INodePtr SceneGraph::root() {
	ASSERT_MESSAGE(!m_rootpath.empty(), "scenegraph root does not exist");
	return m_rootpath.top();
}
  
void SceneGraph::insert_root(scene::INodePtr root) {
	ASSERT_MESSAGE(m_rootpath.empty(), "scenegraph root already exists");

    Node_traverseSubgraph(root, InstanceSubgraphWalker(scene::Path(), 0));

    m_rootpath.push(root);
}
  
void SceneGraph::erase_root() {
    ASSERT_MESSAGE(!m_rootpath.empty(), "scenegraph root does not exist");

    scene::INodePtr root = m_rootpath.top();

    m_rootpath.pop();

    Node_traverseSubgraph(root, UninstanceSubgraphWalker(scene::Path()));
}

void SceneGraph::boundsChanged() {
    m_boundsChanged();
}

void SceneGraph::traverse(const Walker& walker) {
    traverse_subgraph(walker, m_instances.begin());
}

void SceneGraph::traverse_subgraph(const Walker& walker, const scene::Path& start) {
    if(!m_instances.empty()) {
      traverse_subgraph(walker, m_instances.find(PathConstReference(start)));
    }
}

scene::Instance* SceneGraph::find(const scene::Path& path) {
    InstanceMap::iterator i = m_instances.find(PathConstReference(path));
    if(i == m_instances.end()) {
      return NULL;
    }
    return i->second;
}

void SceneGraph::insert(scene::Instance* instance) {
    m_instances.insert(InstanceMap::value_type(PathConstReference(instance->path()), instance));

	// Notify the graph tree model about the change
	sceneChanged();
	
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); i++) {
		(*i)->onSceneNodeInsert(*instance);
	}
	//graph_tree_model_insert(getTreeModel(), *instance);
}

void SceneGraph::erase(scene::Instance* instance) {
  	// Notify the graph tree model about the change
	sceneChanged();
	
	for (ObserverList::iterator i = _sceneObservers.begin(); i != _sceneObservers.end(); i++) {
		(*i)->onSceneNodeErase(*instance);
	}
	//graph_tree_model_erase(scene_graph_get_tree_model(), *instance);

    m_instances.erase(PathConstReference(instance->path()));
}

SignalHandlerId SceneGraph::addBoundsChangedCallback(const SignalHandler& boundsChanged) {
    return m_boundsChanged.connectLast(boundsChanged);
}

void SceneGraph::removeBoundsChangedCallback(SignalHandlerId id) {
    m_boundsChanged.disconnect(id);
}

bool SceneGraph::pre(const Walker& walker, const InstanceMap::iterator& i) {
    return walker.pre(i->first, *i->second);
}

void SceneGraph::post(const Walker& walker, const InstanceMap::iterator& i) {
    walker.post(i->first, *i->second);
}

void SceneGraph::traverse_subgraph(const Walker& walker, InstanceMap::iterator i) {
	std::stack<InstanceMap::iterator> stack;
	if (i != m_instances.end()) {
		// Initialise the start size using the path depth of the given iterator
		const std::size_t startSize = i->first.get().size();
		
		do {
			if (i != m_instances.end() && 
				stack.size() < (i->first.get().size() - startSize + 1))
			{
				stack.push(i);
				++i;
				if (!pre(walker, stack.top())) {
					// Walker's pre() return false, skip subgraph
					while (i != m_instances.end() && 
						   stack.size() < (i->first.get().size() - startSize + 1))
					{
						++i;
					}
				}
			}
			else {
				post(walker, stack.top());
				stack.pop();
			}
		} while (!stack.empty());
	}
}

// RegisterableModule implementation
const std::string& SceneGraph::getName() const {
	static std::string _name("SceneGraph");
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
	
	// Initialise the streams
	const ApplicationContext& ctx = registry.getApplicationContext();
	GlobalOutputStream::instance().setOutputStream(ctx.getOutputStream());
	GlobalErrorStream::instance().setOutputStream(ctx.getErrorStream());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
