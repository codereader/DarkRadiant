#include "LayerManager.h"

#include "i18n.h"
#include "itextstream.h"
#include "imapinfofile.h"
#include "icommandsystem.h"
#include "scene/Node.h"
#include "scenelib.h"
#include "module/StaticModule.h"

#include "AddToLayerWalker.h"
#include "MoveToLayerWalker.h"
#include "RemoveFromLayerWalker.h"
#include "SetLayerSelectedWalker.h"
#include "LayerInfoFileModule.h"

#include <functional>
#include <climits>

namespace scene
{

namespace
{
	const char* const DEFAULT_LAYER_NAME = N_("Default");
	const int DEFAULT_LAYER = 0;
}

LayerManager::LayerManager() :
	_activeLayer(DEFAULT_LAYER)
{
	// Create the "master" layer with ID DEFAULT_LAYER
	createLayer(_(DEFAULT_LAYER_NAME), DEFAULT_LAYER);
}

int LayerManager::createLayer(const std::string& name, int layerID)
{
	// Check if the ID already exists
	if (_layers.find(layerID) != _layers.end())
	{
		// already exists => quit
		return -1;
	}

	// Insert the new layer
	std::pair<LayerMap::iterator, bool> result = _layers.insert(
		LayerMap::value_type(layerID, name)
	);

	if (result.second == false) {
		rError() << "LayerSystem: Could not create layer!" << std::endl;
		return -1;
	}

	// Update the visibility cache, so get the highest ID
	int highestID = getHighestLayerID();

	// Make sure the vector has allocated enough memory
	_layerVisibility.resize(highestID+1);

	// Set the newly created layer to "visible"
	_layerVisibility[result.first->first] = true;

	// Layers have changed
	onLayersChanged();

	// Return the ID of the inserted layer
	return result.first->first;
}

int LayerManager::createLayer(const std::string& name)
{
	// Check if the layer already exists
	int existingID = getLayerID(name);

	if (existingID != -1) {
		rError() << "Could not create layer, name already exists: "
			<< name << std::endl;
		return -1;
	}

	// Layer doesn't exist yet, get the lowest free Id
	int newID = getLowestUnusedLayerID();

	// pass the call to the overload and return
	return createLayer(name, newID);
}

void LayerManager::deleteLayer(const std::string& name)
{
	// Check if the layer already exists
	int layerID = getLayerID(name);

	if (layerID == -1)
	{
		rError() << "Could not delete layer, name doesn't exist: "
			<< name << std::endl;
		return;
	}

	// Remove all nodes from this layer first, but don't de-select them yet
	RemoveFromLayerWalker walker(layerID);
	GlobalSceneGraph().root()->traverse(walker);

	// Remove the layer
	_layers.erase(layerID);

	// Reset the visibility flag to TRUE
	_layerVisibility[layerID] = true;

	if (layerID == _activeLayer)
	{
		// We have removed the active layer, fall back to default
		_activeLayer = DEFAULT_LAYER;
	}

	// Layers have changed
	onLayersChanged();

	// Nodes might have switched to default, fire the visibility 
	// changed event, update the scenegraph and redraw the views
	onNodeMembershipChanged();
}

void LayerManager::foreachLayer(const LayerVisitFunc& visitor)
{
    for (const LayerMap::value_type& pair : _layers)
    {
        visitor(pair.first, pair.second);
    }
}

void LayerManager::reset()
{
	_activeLayer = DEFAULT_LAYER;

	_layers.clear();
	_layers.insert(LayerMap::value_type(DEFAULT_LAYER, _(DEFAULT_LAYER_NAME)));

	_layerVisibility.resize(1);
	_layerVisibility[DEFAULT_LAYER] = true;

	// Update the LayerControlDialog
	_layersChangedSignal.emit();
	_layerVisibilityChangedSignal.emit();
}

bool LayerManager::renameLayer(int layerID, const std::string& newLayerName)
{
	// Check sanity
	if (newLayerName.empty() || newLayerName == _(DEFAULT_LAYER_NAME)) {
		return false; // empty name or default name used
	}

	LayerMap::iterator i = _layers.find(layerID);

	if (i == _layers.end()) {
		return false; // not found
	}

	// Rename that layer
	i->second = newLayerName;

	// Fire the update signal
	onLayersChanged();

	return true;
}

int LayerManager::getFirstVisibleLayer() const
{
	// Iterate over all IDs and check the visibility status, return the first visible
	for (LayerMap::const_iterator i = _layers.begin(); i != _layers.end(); ++i)
	{
		if (_layerVisibility[i->first])
		{
			return i->first;
		}
	}

	// No layer visible, return DEFAULT_LAYER to prevent callers from doing unreasonable things.
	return DEFAULT_LAYER;
}

int LayerManager::getActiveLayer() const
{
	return _activeLayer;
}

void LayerManager::setActiveLayer(int layerID)
{
	LayerMap::iterator i = _layers.find(layerID);

	if (i == _layers.end())
	{
		return; // do nothing
	}

	// ID is valid, assign active layer
	_activeLayer = layerID;
}

bool LayerManager::layerIsVisible(const std::string& layerName) 
{
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		rError() << "Could not query layer visibility, name doesn't exist: "
			<< layerName << std::endl;
		return false;
	}

	return _layerVisibility[layerID];
}

bool LayerManager::layerIsVisible(int layerID) {
	// Sanity check
	if (layerID < 0 || layerID >= static_cast<int>(_layerVisibility.size())) {
		rMessage() << "LayerSystem: Querying invalid layer ID: " << layerID << std::endl;
		return false;
	}

	return _layerVisibility[layerID];
}

void LayerManager::setLayerVisibility(int layerID, bool visible)
{
	// Sanity check
	if (layerID < 0 || layerID >= static_cast<int>(_layerVisibility.size()))
	{
		rMessage() <<
			"LayerSystem: Setting visibility of invalid layer ID: " <<
			layerID << std::endl;
		return;
	}

	// Set the visibility
	_layerVisibility[layerID] = visible;

	if (!visible && layerID == _activeLayer)
	{
		// We just hid the active layer, fall back to another one
		_activeLayer = getFirstVisibleLayer();
	}
    
    // If the active layer is hidden (which can occur after "hide all")
    // re-set the active layer to this one as it has been made visible
    if (visible && _activeLayer < static_cast<int>(_layerVisibility.size()) && 
        !_layerVisibility[_activeLayer])
    {
        _activeLayer = layerID;
    }

	// Fire the visibility changed event
	onLayerVisibilityChanged();
}

void LayerManager::setLayerVisibility(const std::string& layerName, bool visible) 
{
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) 
	{
		rError() << "Could not set layer visibility, name doesn't exist: " << layerName << std::endl;
		return;
	}

	// Pass the call to the overloaded method to do the work
	setLayerVisibility(layerID, visible);
}

void LayerManager::updateSceneGraphVisibility()
{
	UpdateNodeVisibilityWalker walker(GlobalSceneGraph().root());
	GlobalSceneGraph().root()->traverseChildren(walker);

	// Redraw
	SceneChangeNotify();
}

void LayerManager::onLayersChanged()
{
	_layersChangedSignal.emit();
}

void LayerManager::onNodeMembershipChanged()
{
	_nodeMembershipChangedSignal.emit();

	updateSceneGraphVisibility();
}

void LayerManager::onLayerVisibilityChanged()
{
	// Update all nodes and views
	updateSceneGraphVisibility();

	// Update the LayerControlDialog
	_layerVisibilityChangedSignal.emit();
}

void LayerManager::addSelectionToLayer(int layerID) {
	// Check if the layer ID exists
	if (_layers.find(layerID) == _layers.end()) {
		return;
	}

	// Instantiate a Selectionwalker and traverse the selection
	AddToLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	onNodeMembershipChanged();
}

void LayerManager::addSelectionToLayer(const std::string& layerName) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		rError() << "Cannot add to layer, name doesn't exist: "
			<< layerName << std::endl;
		return;
	}

	// Pass the call to the overload
	addSelectionToLayer(layerID);
}

void LayerManager::moveSelectionToLayer(const std::string& layerName) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		rError() << "Cannot move to layer, name doesn't exist: "
			<< layerName << std::endl;
		return;
	}

	// Pass the call to the overload
	moveSelectionToLayer(layerID);
}

void LayerManager::moveSelectionToLayer(int layerID) {
	// Check if the layer ID exists
	if (_layers.find(layerID) == _layers.end()) {
		return;
	}

	// Instantiate a Selectionwalker and traverse the selection
	MoveToLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	onNodeMembershipChanged();
}

void LayerManager::removeSelectionFromLayer(const std::string& layerName) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		rError() << "Cannot remove from layer, name doesn't exist: "
			<< layerName << std::endl;
		return;
	}

	// Pass the call to the overload
	removeSelectionFromLayer(layerID);
}

void LayerManager::removeSelectionFromLayer(int layerID) {
	// Check if the layer ID exists
	if (_layers.find(layerID) == _layers.end()) {
		return;
	}

	// Instantiate a Selectionwalker and traverse the selection
	RemoveFromLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	onNodeMembershipChanged();
}

bool LayerManager::updateNodeVisibility(const scene::INodePtr& node)
{
    if (!node->supportsStateFlag(Node::eLayered))
    {
        return true; // doesn't support layers, return true for visible
    }

	// Get the list of layers the node is associated with
	const auto& layers = node->getLayers();

	// We start with the assumption that a node is hidden
    bool isHidden = true;

	// Cycle through the Node's layers, and show the node as soon as
	// a visible layer is found.
	for (int layerId : layers)
	{
		if (_layerVisibility[layerId])
		{
			// The layer is visible, set the visibility to true and quit
            isHidden = false;
			break;
		}
	}

    if (isHidden)
    {
        node->enable(Node::eLayered);
    }
    else
    {
        node->disable(Node::eLayered);
    }

	// If node is hidden, return FALSE
	return !isHidden;
}

void LayerManager::setSelected(int layerID, bool selected)
{
	SetLayerSelectedWalker walker(layerID, selected);

    if (GlobalSceneGraph().root())
    {
        GlobalSceneGraph().root()->traverseChildren(walker);
    }
}

sigc::signal<void> LayerManager::signal_layersChanged()
{
	return _layersChangedSignal;
}

sigc::signal<void> LayerManager::signal_layerVisibilityChanged()
{
	return _layerVisibilityChangedSignal;
}

sigc::signal<void> LayerManager::signal_nodeMembershipChanged()
{
	return _nodeMembershipChangedSignal;
}

int LayerManager::getLayerID(const std::string& name) const
{
	for (LayerMap::const_iterator i = _layers.begin(); i != _layers.end(); i++) {
		if (i->second == name) {
			// Name found, return the ID
			return i->first;
		}
	}

	return -1;
}

std::string LayerManager::getLayerName(int layerID) const 
{
	LayerMap::const_iterator found = _layers.find(layerID);

	if (found != _layers.end()) {
		return found->second;
	}

	// not found
	return "";
}

bool LayerManager::layerExists(int layerID) const
{
	return _layers.find(layerID) != _layers.end();
}

int LayerManager::getHighestLayerID() const
{
	if (_layers.size() == 0) {
		// Empty layer map, just return DEFAULT_LAYER
		return DEFAULT_LAYER;
	}

	// A map is sorted, so return the ID of the element from the end of the map
	return _layers.rbegin()->first;
}

int LayerManager::getLowestUnusedLayerID()
{
	for (int i = 0; i < INT_MAX; i++) {
		if (_layers.find(i) == _layers.end()) {
			// Found a free ID
			return i;
		}
	}

	return -1;
}

} // namespace scene
