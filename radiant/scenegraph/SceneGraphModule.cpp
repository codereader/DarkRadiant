#include "SceneGraphModule.h"

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

SceneGraphAPI::SceneGraphAPI() {
	_sceneGraph = CompiledGraphPtr(new CompiledGraph());
}

scene::Graph* SceneGraphAPI::getTable() {
	// Return the contained pointer to the CompiledGraph
	return _sceneGraph.get();
}

// ------------ Module Definition -------------------------------------

typedef SingletonModule<SceneGraphAPI> SceneGraphModule;
typedef Static<SceneGraphModule> StaticSceneGraphModule;
StaticRegisterModule staticRegisterSceneGraph(StaticSceneGraphModule::instance());

GraphTreeModel* scene_graph_get_tree_model() {
	CompiledGraph* sceneGraph = static_cast<CompiledGraph*>(
		StaticSceneGraphModule::instance().getTable()
	);
	return sceneGraph->getTreeModel();
}
