#include "LayerManager.h"

#include "i18n.h"
#include "itextstream.h"
#include "scene/Node.h"
#include "scenelib.h"
#include "module/StaticModule.h"

#include "AddToLayerWalker.h"
#include "MoveToLayerWalker.h"
#include "RemoveFromLayerWalker.h"
#include "SetLayerSelectedWalker.h"

#include <functional>
#include <climits>

namespace scene
{

namespace
{
	constexpr const char* const DEFAULT_LAYER_NAME = N_("Default");
	constexpr int DEFAULT_LAYER = 0;
	constexpr int NO_PARENT_ID = -1;
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
	if (_layers.count(layerID) > 0)
	{
		// already exists => quit
		return -1;
	}

	// Insert the new layer
	auto result = _layers.emplace(layerID, name);

	if (result.second == false)
    {
		rError() << "LayerSystem: Could not create layer!" << std::endl;
		return -1;
	}

	// Update the visibility cache, so get the highest ID
	int highestID = getHighestLayerID();

	// Make sure the vectors are large enough
	_layerVisibility.resize(highestID+1);
	_layerParentIds.resize(highestID+1);

	// Set the newly created layer to "visible"
	_layerVisibility[layerID] = true;
    _layerParentIds[layerID] = NO_PARENT_ID;

	// Layers have changed
	onLayersChanged();

	// Return the ID of the inserted layer
	return layerID;
}

int LayerManager::createLayer(const std::string& name)
{
	// Check if the layer already exists
	int existingID = getLayerID(name);

	if (existingID != -1)
    {
		rError() << "Could not create layer, name already exists: " << name << std::endl;
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
		rError() << "Could not delete layer, name doesn't exist: " << name << std::endl;
		return;
	}

    if (layerID == DEFAULT_LAYER)
    {
        rError() << "Cannot delete the default layer" << std::endl;
        return;
    }

	// Remove all nodes from this layer first, but don't de-select them yet
	RemoveFromLayerWalker walker(layerID);
	GlobalSceneGraph().root()->traverse(walker);

	// Remove the layer
	_layers.erase(layerID);

	// Reset the visibility flag to TRUE, remove parent
	_layerVisibility[layerID] = true;
	_layerParentIds[layerID] = NO_PARENT_ID;

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
    for (const auto& pair : _layers)
    {
        visitor(pair.first, pair.second);
    }
}

void LayerManager::reset()
{
	_activeLayer = DEFAULT_LAYER;

	_layers.clear();
	_layers.emplace(DEFAULT_LAYER, _(DEFAULT_LAYER_NAME));

	_layerVisibility.resize(1);
	_layerVisibility[DEFAULT_LAYER] = true;

    _layerParentIds.resize(1);
    _layerParentIds[DEFAULT_LAYER] = NO_PARENT_ID;

	// Emit all changed signals
	_layersChangedSignal.emit();
	_layerVisibilityChangedSignal.emit();
	_layerHierarchyChangedSignal.emit();
}

bool LayerManager::renameLayer(int layerID, const std::string& newLayerName)
{
	// Check sanity
	if (newLayerName.empty() || newLayerName == _(DEFAULT_LAYER_NAME)) {
		return false; // empty name or default name used
	}

	auto i = _layers.find(layerID);

	if (i == _layers.end())
    {
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
	for (const auto& [layerId, _] : _layers)
	{
		if (_layerVisibility[layerId])
		{
			return layerId;
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
	if (_layers.count(layerID) == 0)
	{
		return; // do nothing
	}

	// ID is valid, assign active layer
	_activeLayer = layerID;
}

bool LayerManager::layerIsVisible(int layerID)
{
	// Sanity check
	if (layerID < 0 || layerID >= static_cast<int>(_layerVisibility.size()))
    {
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
		rMessage() << "LayerSystem: Setting visibility of invalid layer ID: " << layerID << std::endl;
		return;
	}

    if (_layerVisibility[layerID] == visible)
    {
        return; // nothing to change here
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

	// Update the LegacyLayerControlDialog
	_layerVisibilityChangedSignal.emit();
}

void LayerManager::addSelectionToLayer(int layerID)
{
	// Check if the layer ID exists
    if (_layers.count(layerID) == 0) return;

	// Instantiate a Selectionwalker and traverse the selection
	AddToLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	onNodeMembershipChanged();
}

void LayerManager::moveSelectionToLayer(int layerID)
{
	// Check if the layer ID exists
    if (_layers.count(layerID) == 0) return;

	// Instantiate a Selectionwalker and traverse the selection
	MoveToLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	onNodeMembershipChanged();
}

void LayerManager::removeSelectionFromLayer(int layerID)
{
	// Check if the layer ID exists
    if (_layers.count(layerID) == 0) return;

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

int LayerManager::getParentLayer(int layerId)
{
    return _layerParentIds.at(layerId);
}

void LayerManager::setParentLayer(int childLayerId, int parentLayerId)
{
    if (childLayerId == DEFAULT_LAYER && parentLayerId != -1)
    {
        throw std::invalid_argument("Cannot assign a parent to the default layer");
    }

    // Non-existent layer IDs will throw
    if (!layerExists(childLayerId) || 
        parentLayerId != -1 && !layerExists(parentLayerId))
    {
        throw std::invalid_argument("Invalid layer ID");
    }

    if (childLayerId == parentLayerId)
    {
        throw std::invalid_argument("Cannot assign a layer as parent of itself");
    }

    // Detect recursions, if any parent layer has this layer in its hierarchy, we should throw
    if (layerIsChildOf(parentLayerId, childLayerId))
    {
        throw std::invalid_argument("This relationship change would result in a recursion");
    }

    if (_layerParentIds.at(childLayerId) != parentLayerId)
    {
        _layerParentIds.at(childLayerId) = parentLayerId;
        _layerHierarchyChangedSignal.emit();
    }
}

bool LayerManager::layerIsChildOf(int candidateLayerId, int parentLayerId)
{
    // Nothing is a parent of the null layer
    if (candidateLayerId == NO_PARENT_ID || parentLayerId == NO_PARENT_ID)
    {
        return false;
    }

    // Check the hierarchy of the candidate
    for (int immediateParentId = getParentLayer(candidateLayerId); 
         immediateParentId != NO_PARENT_ID;
         immediateParentId = getParentLayer(immediateParentId))
    {
        if (immediateParentId == parentLayerId)
        {
            return true;
        }
    }

    return false;
}

sigc::signal<void> LayerManager::signal_layersChanged()
{
	return _layersChangedSignal;
}

sigc::signal<void> LayerManager::signal_layerVisibilityChanged()
{
	return _layerVisibilityChangedSignal;
}

sigc::signal<void> LayerManager::signal_layerHierarchyChanged()
{
	return _layerHierarchyChangedSignal;
}

sigc::signal<void> LayerManager::signal_nodeMembershipChanged()
{
	return _nodeMembershipChangedSignal;
}

int LayerManager::getLayerID(const std::string& name) const
{
	for (const auto& [layerId, layerName] : _layers)
    {
		if (layerName == name) {
			// Name found, return the ID
			return layerId;
		}
	}

	return -1;
}

std::string LayerManager::getLayerName(int layerID) const 
{
	auto found = _layers.find(layerID);

	return found != _layers.end() ? found->second : std::string();
}

bool LayerManager::layerExists(int layerID) const
{
	return _layers.count(layerID) > 0;
}

int LayerManager::getHighestLayerID() const
{
	if (_layers.size() == 0)
    {
		// Empty layer map, just return DEFAULT_LAYER
		return DEFAULT_LAYER;
	}

	// A map is sorted, so return the ID of the element from the end of the map
	return _layers.rbegin()->first;
}

int LayerManager::getLowestUnusedLayerID()
{
	for (int i = 0; i < INT_MAX; i++)
    {
		if (_layers.count(i) == 0)
        {
			// Found a free ID
			return i;
		}
	}

	return -1;
}

} // namespace scene
