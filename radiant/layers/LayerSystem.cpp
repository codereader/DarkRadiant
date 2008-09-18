#include "LayerSystem.h"

#include "ieventmanager.h"
#include "stream/textstream.h"
#include "scene/Node.h"
#include "modulesystem/StaticModule.h"

#include "AddToLayerWalker.h"
#include "MoveToLayerWalker.h"
#include "RemoveFromLayerWalker.h"
#include "SetLayerSelectedWalker.h"

#include "ui/layers/LayerControlDialog.h"

namespace scene {

	namespace {
		const std::string DEFAULT_LAYER_NAME("Default");
	} 

int LayerSystem::createLayer(const std::string& name, int layerID) {
	// Check if the ID already exists
	if (_layers.find(layerID) != _layers.end()) {
		// already exists => quit
		return -1;
	}

	// Insert the new layer
	std::pair<LayerMap::iterator, bool> result = _layers.insert(
		LayerMap::value_type(layerID, name)
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

	// pass the call to the overload and return
	return createLayer(name, newID);
}

void LayerSystem::deleteLayer(const std::string& name) {
	// Check if the layer already exists
	int layerID = getLayerID(name);

	if (layerID == -1) {
		globalErrorStream() << "Could not delete layer, name doesn't exist: " 
			<< name.c_str() << "\n";
		return;
	}

	// Remove all nodes from this layer first, but don't de-select them yet
	RemoveFromLayerWalker walker(layerID);
	GlobalSceneGraph().traverse(walker);

	// Remove the layer
	_layers.erase(layerID);

	// Reset the visibility flag to TRUE
	_layerVisibility[layerID] = true;

	// Fire the visibility changed event to
	// update the scenegraph and redraw the views
	onLayerVisibilityChanged();
}

void LayerSystem::foreachLayer(Visitor& visitor) {
	for (LayerMap::iterator i = _layers.begin(); i != _layers.end(); i++) {
		visitor.visit(i->first, i->second);
	}
}

void LayerSystem::reset() {
	_layers.clear();
	_layers.insert(LayerMap::value_type(0, DEFAULT_LAYER_NAME));

	_layerVisibility.resize(1);
	_layerVisibility[0] = true;

	// Update the LayerControlDialog
	ui::LayerControlDialog::Instance().refresh();
}

int LayerSystem::getFirstVisibleLayer() const {
	// Iterate over all IDs and check the visibility status, return the first visible
	for (LayerMap::const_iterator i = _layers.begin(); i != _layers.end(); i++) {
		if (_layerVisibility[i->first]) {
			return i->first;
		}
	}

	// No layer visible, return
	return -1;
}

bool LayerSystem::layerIsVisible(const std::string& layerName) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		globalErrorStream() << "Could not query layer visibility, name doesn't exist: " 
			<< layerName.c_str() << "\n";
		return false;
	}

	return _layerVisibility[layerID];
}

bool LayerSystem::layerIsVisible(int layerID) {
	// Sanity check
	if (layerID < 0 || layerID >= static_cast<int>(_layerVisibility.size())) {
		globalOutputStream() << "LayerSystem: Querying invalid layer ID: " << layerID << "\n";
		return false;
	}

	return _layerVisibility[layerID];
}

void LayerSystem::setLayerVisibility(int layerID, bool visible) {
	// Sanity check
	if (layerID < 0 || layerID >= static_cast<int>(_layerVisibility.size())) {
		globalOutputStream() << 
			"LayerSystem: Setting visibility of invalid layer ID: " <<
			layerID << "\n";
		return;
	}

	// Set the visibility
	_layerVisibility[layerID] = visible;

	// Fire the visibility changed event
	onLayerVisibilityChanged();
}

void LayerSystem::setLayerVisibility(const std::string& layerName, bool visible) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		globalErrorStream() << "Could not set layer visibility, name doesn't exist: " 
			<< layerName.c_str() << "\n";
		return;
	}

	// Pass the call to the overloaded method to do the work
	setLayerVisibility(layerID, visible);
}

void LayerSystem::updateSceneGraphVisibility() {
	UpdateNodeVisibilityWalker walker;
	GlobalSceneGraph().root()->traverse(walker);
}

void LayerSystem::onLayerVisibilityChanged() {
	// Update all nodes
	updateSceneGraphVisibility();

	// Redraw
	SceneChangeNotify();

	// Update the LayerControlDialog
	ui::LayerControlDialog::Instance().update();
}

void LayerSystem::addSelectionToLayer(int layerID) {
	// Check if the layer ID exists
	if (_layers.find(layerID) == _layers.end()) {
		return;
	}

	// Instantiate a Selectionwalker and traverse the selection
	AddToLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	updateSceneGraphVisibility();
}

void LayerSystem::addSelectionToLayer(const std::string& layerName) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		globalErrorStream() << "Cannot add to layer, name doesn't exist: " 
			<< layerName.c_str() << "\n";
		return;
	}

	// Pass the call to the overload
	addSelectionToLayer(layerID);
}

void LayerSystem::moveSelectionToLayer(const std::string& layerName) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		globalErrorStream() << "Cannot move to layer, name doesn't exist: " 
			<< layerName.c_str() << "\n";
		return;
	}

	// Pass the call to the overload
	moveSelectionToLayer(layerID);
}

void LayerSystem::moveSelectionToLayer(int layerID) {
	// Check if the layer ID exists
	if (_layers.find(layerID) == _layers.end()) {
		return;
	}

	// Instantiate a Selectionwalker and traverse the selection
	MoveToLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	updateSceneGraphVisibility();
}

void LayerSystem::removeSelectionFromLayer(const std::string& layerName) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		globalErrorStream() << "Cannot remove from layer, name doesn't exist: " 
			<< layerName.c_str() << "\n";
		return;
	}

	// Pass the call to the overload
	removeSelectionFromLayer(layerID);
}

void LayerSystem::removeSelectionFromLayer(int layerID) {
	// Check if the layer ID exists
	if (_layers.find(layerID) == _layers.end()) {
		return;
	}

	// Instantiate a Selectionwalker and traverse the selection
	RemoveFromLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	updateSceneGraphVisibility();
}

bool LayerSystem::updateNodeVisibility(const scene::INodePtr& node) {
	// Get the list of layers the node is associated with
	// greebo: TODO: Check if returning the LayerList by value is taxing.
	LayerList layers = node->getLayers();

	// We start with the assumption that a node is hidden
	node->enable(Node::eLayered);

	// Cycle through the Node's layers, and show the node as soon as 
	// a visible layer is found.
	for (LayerList::const_iterator i = layers.begin(); i != layers.end(); i++) {
		if (_layerVisibility[*i]) {
			// The layer is visible, set the visibility to true and quit
			node->disable(Node::eLayered);
			return true;
		}
	}

	// Node is hidden, return FALSE
	return false;
}

void LayerSystem::setSelected(int layerID, bool selected) {
	SetLayerSelectedWalker walker(layerID, selected);
	GlobalSceneGraph().root()->traverse(walker);
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

std::string LayerSystem::getLayerName(int layerID) const {
	LayerMap::const_iterator found = _layers.find(layerID);

	if (found != _layers.end()) {
		return found->second;
	}

	// not found
	return "";
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
	
	// Create the "master" layer with ID 0
	createLayer(DEFAULT_LAYER_NAME);

	// Add command targets for the first 10 layer IDs here
	for (int i = 0; i < 10; i++) {
		_commandTargets.push_back(
			LayerCommandTargetPtr(new LayerCommandTarget(i))
		);
	}

	GlobalEventManager().addCommand(
		"ToggleLayerControlDialog", 
		FreeCaller<ui::LayerControlDialog::toggle>()
	);
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
