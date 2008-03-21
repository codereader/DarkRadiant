#include "LayerSystem.h"

#include "ieventmanager.h"
#include "stream/textstream.h"
#include "scene/Node.h"
#include "modulesystem/StaticModule.h"
#include "AddToLayerWalker.h"
#include "UpdateNodeVisibilityWalker.h"

namespace scene {

LayerSystem::LayerSystem() {
	_layerVisibility.resize(2); // temporary
	_layerVisibility[0] = true;
	_layerVisibility[1] = true;
}

bool LayerSystem::layerIsVisible(const std::string& layerName) {
	return _layerVisibility[1];
}

void LayerSystem::setLayerVisibility(const std::string& layerName, bool visible) {
	_layerVisibility[1] = visible;

	layerVisibilityChanged();
}

void LayerSystem::layerVisibilityChanged() {
	UpdateNodeVisibilityWalker walker;
	GlobalSceneGraph().traverse(walker);

	// Redraw
	SceneChangeNotify();
}

void LayerSystem::addSelectionToLayer(const std::string& layerName) {
	// Instantiate a Selectionwalker
	AddToLayerWalker walker(1);
	GlobalSelectionSystem().foreachSelected(walker);
}

bool LayerSystem::updateNodeVisibility(const scene::INodePtr& node) {
	// Get the list of layers the node is associated with
	// greebo: TODO: Check if returning the LayerList by value is taxing.
	LayerList layers = node->getLayers();

	// We start with the assumption that a node is visible
	node->disable(Node::eLayered);

	// Cycle through the Node's layers, and hide the node as soon as 
	// a hidden layer is found.
	for (LayerList::const_iterator i = layers.begin(); i != layers.end(); i++) {
		if (!_layerVisibility[*i]) {
			// The layer is invisible, set the visibility to false and quit
			node->enable(Node::eLayered);
			return false;
		}
	}

	// Node is visible, return TRUE
	return true;
}

void LayerSystem::toggleLayerVisibility() {
	setLayerVisibility("", !_layerVisibility[1]);
}

void LayerSystem::addSelectionToLayer1() {
	addSelectionToLayer("");
}

// RegisterableModule implementation
const std::string& LayerSystem::getName() const {
	static std::string _name(MODULE_LAYERSYSTEM);
	return _name;
}

const StringSet& LayerSystem::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_EVENTMANAGER);
	}

	return _dependencies;
}

void LayerSystem::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "LayerSystem::initialiseModule called.\n";
	
	// Add command targets here
	GlobalEventManager().addCommand("LayerToggleVisibility", ToggleCaller(*this));
	GlobalEventManager().addCommand("LayerAddSelectionToLayer1", AddSelectionCaller(*this));
}

void LayerSystem::shutdownModule() {
	
}

// Define the static LayerSystem module
module::StaticModule<LayerSystem> layerSystemModule;

} // namespace scene
