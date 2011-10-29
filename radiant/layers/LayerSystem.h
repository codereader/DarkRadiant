#pragma once

#include <vector>
#include <map>
#include "ilayer.h"
#include "LayerCommandTarget.h"

namespace scene {

namespace
{
	const char* const COMMAND_PREFIX_ADDTOLAYER("AddSelectionToLayer");
	const char* const COMMAND_PREFIX_MOVETOLAYER("MoveSelectionToLayer");
	const char* const COMMAND_PREFIX_SHOWLAYER("ShowLayer");
	const char* const COMMAND_PREFIX_HIDELAYER("HideLayer");
}

class LayerSystem :
	public ILayerSystem
{
private:
	// greebo: An array of booleans reflects the visibility status
	// of all layers. Indexed by the layer id, it can be used to
	// quickly check whether a layer is visible or not.
	typedef std::vector<bool> LayerVisibilityList;
	LayerVisibilityList _layerVisibility;

	// The list of named layers, indexed by an integer ID
	typedef std::map<int, std::string> LayerMap;
	LayerMap _layers;

	typedef std::vector<LayerCommandTargetPtr> CommandTargetList;
	CommandTargetList _commandTargets;

	// The ID of the active layer
	int _activeLayer;

public:
	LayerSystem();

	/**
	 * greebo: Creates a new layer with the given name.
	 *
	 * @returns: the ID of the layer of -1 if the layer could not
	 *           be created (e.g. due to a name conflict).
	 */
	int createLayer(const std::string& name);

	/**
	 * greebo: Overload: Creates a new layer with the given name and the given ID.
	 *
	 * @returns: the ID of the layer of -1 if the layer could not
	 *           be created (e.g. due to a name/ID conflict).
	 */
	int createLayer(const std::string& name, int layerID);

	/**
	 * greebo: Deletes the named layer. All nodes are removed
	 *         from this layer before deletion.
	 */
	void deleteLayer(const std::string& name);

	/**
	 * greebo: Resets the layer system into its ground state. Deletes all
	 *         layers except for layer #0 which is renamed to "Default".
	 */
	void reset();

	/**
	 * greebo: Visits each layer using the given visitor.
	 */
	void foreachLayer(Visitor& visitor);

	// Returns the ID of the named layer, or -1 if name doesn't exist
	int getLayerID(const std::string& name) const;
	std::string getLayerName(int layerID) const;

	// Check for layer existence
	bool layerExists(int layerID) const;

	// Renames the given layer and returns TRUE on success
	bool renameLayer(int layerID, const std::string& newLayerName);

	// Returns the ID of the first visible layer or -1 if all are hidden.
	int getFirstVisibleLayer() const;

	// Active layers
	int getActiveLayer() const;
	void setActiveLayer(int layerID);

	// Returns true if the given layer is visible
	bool layerIsVisible(const std::string& layerName);
	bool layerIsVisible(int layerID);

	// Sets the visibility state of the given layer to <visible>
	void setLayerVisibility(const std::string& layerName, bool visible);
	void setLayerVisibility(int layerID, bool visible);

	/**
	 * greebo: Traverses the selection and adds each node to the given layer.
	 */
	void addSelectionToLayer(const std::string& layerName);
	void addSelectionToLayer(int layerID);

	/**
	 * greebo: Moves all selected nodes to the given layer. This implicitly
	 *         removes the nodes from all other layers.
	 */
	void moveSelectionToLayer(const std::string& layerName);
	void moveSelectionToLayer(int layerID);

	/**
	 * greebo: Removes the selected nodes from the given layers.
	 */
	void removeSelectionFromLayer(const std::string& layerName);
	void removeSelectionFromLayer(int layerID);

	bool updateNodeVisibility(const scene::INodePtr& node);

	// Selects/unselects an entire layer
	void setSelected(int layerID, bool selected);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);

	// Command target
	void createLayerCmd(const cmd::ArgumentList& args);

private:
	// Internal event, updates the scenegraph
	void onLayerVisibilityChanged();

	// Updates the visibility state of the entire scenegraph
	void updateSceneGraphVisibility();

	// Returns the highest used layer Id
	int getHighestLayerID() const;

	// Returns the lowest unused layer ID
	int getLowestUnusedLayerID();
};

// Internal accessor, only accessible within this binary
LayerSystem& getLayerSystem();

} // namespace scene
