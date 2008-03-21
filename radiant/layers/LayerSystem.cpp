#include "LayerSystem.h"

#include "ieventmanager.h"
#include "stream/textstream.h"
#include "scene/Node.h"
#include "modulesystem/StaticModule.h"
#include "AddToLayerWalker.h"
#include "UpdateNodeVisibilityWalker.h"

namespace scene {

LayerSystem::LayerSystem() {
	// make sure we have room for 2 layers right from the start
	_layerVisibility.resize(2); 
	_layerVisibility[0] = true;
	_layerVisibility[1] = true;
}

int LayerSystem::createLayer(const std::string& name) {
	// Check if the layer already exists
	int existingID = getLayerID(name);

	if (existingID != -1) {
		globalErrorStream() << "Could not create layer, name already exists: " 
			<< name.c_str() << "\n";
		return -1;
	}

	// Layer doesn't exist yet, get the lowest free Id
	int newID = getLowestUnusedLayerID();

	std::pair<LayerMap::iterator, bool> result = _layers.insert(
		LayerMap::value_type(newID, name)
	);

	if (result.second == false) {
		globalErrorStream() << "LayerSystem: Could not create layer!\n";
		return -1;
	}

	// Update the visibility cache, so get the highest ID
	int highestID = getHighestLayerID();

	// Make sure the vector has allocated enough memory
	_layerVisibility.resize(highestID+1);

	// Set the newly created layer to "visible"
	_layerVisibility[result.first->first] = true;
	
	// Return the ID of the inserted layer
	return result.first->first;
}

void LayerSystem::deleteLayer(const std::string& name) {

}

bool LayerSystem::layerIsVisible(const std::string& layerName) {
	return _layerVisibility[1];
}

void LayerSystem::setLayerVisibility(const std::string& layerName, bool visible) {
	_layerVisibility[1] = visible;

	// Fire the visibility changed event
	onLayerVisibilityChanged();
}

void LayerSystem::onLayerVisibilityChanged() {
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

int LayerSystem::getLayerID(const std::string& name) const {
	for (LayerMap::const_iterator i = _layers.begin(); i != _layers.end(); i++) {
		if (i->second == name) {
			// Name found, return the ID
			return i->first;
		}
	}

	return -1;
}

int LayerSystem::getHighestLayerID() const {
	if (_layers.size() == 0) {
		// Empty layer map, just return 0
		return 0;
	}

	// A map is sorted, so return the ID of the element from the end of the map
	return _layers.rbegin()->first;
}

int LayerSystem::getLowestUnusedLayerID() {
	for (int i = 0; i < INT_MAX; i++) {
		if (_layers.find(i) == _layers.end()) {
			// Found a free ID
			return i;
		}
	}

	return -1;
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

// Internal accessor method
LayerSystem& getLayerSystem() {
	return *layerSystemModule.getModule();
}

} // namespace scene
