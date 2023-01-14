#pragma once

#include <vector>
#include <map>
#include "ilayer.h"

namespace scene 
{

class LayerManager :
	public ILayerManager
{
private:
    INode& _rootNode;

    // The list of named layers, indexed by an integer ID
    std::map<int, std::string> _layers;

	// greebo: An array of booleans reflects the visibility status
	// of all layers. Indexed by the layer id, it can be used to
	// quickly check whether a layer is visible or not.
    std::vector<bool> _layerVisibility;

    // The parent IDs of each layer (-1 for no parent)
    std::vector<int> _layerParentIds;

	// The ID of the active layer
	int _activeLayer;

	sigc::signal<void> _layersChangedSignal;
	sigc::signal<void> _layerVisibilityChangedSignal;
	sigc::signal<void> _layerHierarchyChangedSignal;
	sigc::signal<void> _nodeMembershipChangedSignal;

public:
	LayerManager(INode& rootNode);

	/**
	 * greebo: Creates a new layer with the given name.
	 *
	 * @returns: the ID of the layer of -1 if the layer could not
	 *           be created (e.g. due to a name conflict).
	 */
	int createLayer(const std::string& name) override;

	/**
	 * greebo: Overload: Creates a new layer with the given name and the given ID.
	 *
	 * @returns: the ID of the layer of -1 if the layer could not
	 *           be created (e.g. due to a name/ID conflict).
	 */
	int createLayer(const std::string& name, int layerID) override;

	/**
	 * greebo: Deletes the named layer. All nodes are removed
	 *         from this layer before deletion.
	 */
	void deleteLayer(const std::string& name) override;

	/**
	 * greebo: Resets the layer system into its ground state. Deletes all
	 *         layers except for layer #0 which is renamed to "Default".
	 */
	void reset() override;

	/**
	 * greebo: Visits each layer using the given visitor.
	 */
    void foreachLayer(const LayerVisitFunc& visitor) override;

	// Returns the ID of the named layer, or -1 if name doesn't exist
	int getLayerID(const std::string& name) const override;
	std::string getLayerName(int layerID) const override;

	// Check for layer existence
	bool layerExists(int layerID) const override;

	// Renames the given layer and returns TRUE on success
	bool renameLayer(int layerID, const std::string& newLayerName) override;

	// Returns the ID of the first visible layer or -1 if all are hidden.
	int getFirstVisibleLayer() const override;

	// Active layers
	int getActiveLayer() const override;
	void setActiveLayer(int layerID) override;

	// Returns true if the given layer is visible
	bool layerIsVisible(int layerID) override;

	// Sets the visibility state of the given layer to <visible>
	void setLayerVisibility(int layerID, bool visible) override;

	/**
	 * greebo: Traverses the selection and adds each node to the given layer.
	 */
	void addSelectionToLayer(int layerID) override;

	/**
	 * greebo: Moves all selected nodes to the given layer. This implicitly
	 *         removes the nodes from all other layers.
	 */
	void moveSelectionToLayer(int layerID) override;

	/**
	 * greebo: Removes the selected nodes from the given layers.
	 */
	void removeSelectionFromLayer(int layerID) override;

	bool updateNodeVisibility(const scene::INodePtr& node) override;

	// Selects/unselects an entire layer
	void setSelected(int layerID, bool selected) override;

    int getParentLayer(int layerId) override;
    void setParentLayer(int childLayerId, int parentLayerId) override;
    bool layerIsChildOf(int candidateLayerId, int parentLayerId) override;

	sigc::signal<void> signal_layersChanged() override;
	sigc::signal<void> signal_layerVisibilityChanged() override;
	sigc::signal<void> signal_layerHierarchyChanged() override;
	sigc::signal<void> signal_nodeMembershipChanged() override;

private:
    // Recursively sets the visibility of the given layer and updates
    // the flags on the _layerVisibility vector.
    // Returns true if any flag changed, false if nothing changed.
    bool setLayerVisibilityRecursively(int layerID, bool visible);

    // Invokes the function object with each layer ID in the hierarchy, including the given root
    void foreachLayerInHierarchy(int rootLayerId, const std::function<void(int)>& functor);

	// Internal event emitter
	void onLayersChanged();

	// Internal event, updates the scenegraph
	void onLayerVisibilityChanged();

	// Internal event emitter
	void onNodeMembershipChanged();

	// Updates the visibility state of the entire scenegraph
	void updateSceneGraphVisibility();

	// Returns the highest used layer Id
	int getHighestLayerID() const;

	// Returns the lowest unused layer ID
	int getLowestUnusedLayerID();
};

} // namespace scene
