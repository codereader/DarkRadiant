#pragma once

#include <set>
#include <string>
#include <functional>
#include "imodule.h"
#include <sigc++/signal.h>

namespace scene
{

class INode;
typedef std::shared_ptr<INode> INodePtr;

// A list of named layers
typedef std::set<int> LayerList;

/**
 * greebo: Interface of a Layered object.
 */
class Layered
{
public:
	// Destructor
	virtual ~Layered() {}

	/**
     * Add this object to the layer with the given ID.
     */
    virtual void addToLayer(int layerId) = 0;

	/**
     * Moves this object to the layer. After this operation,the item is member
	 * of this layer only, all other layer memberships are erased.
     */
    virtual void moveToLayer(int layerId) = 0;

    /**
     * Remove this object from the layer with the given ID.
     */
    virtual void removeFromLayer(int layerId) = 0;

    /**
     * Return the set of layers to which this object is assigned.
     */
    virtual const LayerList& getLayers() const = 0;

	/**
	 * greebo: This assigns the given node to the given set of layers. Any previous
	 * assignments of the node will be overwritten by this routine.
	 * Note: The newLayers list must not be empty, otherwise the call will be ignored.
	 */
	virtual void assignToLayers(const LayerList& newLayers) = 0;
};

class ILayerManager
{
public:
	typedef std::shared_ptr<ILayerManager> Ptr;

	virtual ~ILayerManager() {}

	/**
	 * greebo: Creates a new layer with the given name.
	 *
	 * @returns: the ID of the layer of -1 if the layer could not
	 * be created (e.g. due to a name conflict).
	 */
	virtual int createLayer(const std::string& name) = 0;

	/**
	 * greebo: Overload: Creates a new layer with the given name and the given ID.
	 *
	 * @returns: the ID of the layer of -1 if the layer could not
	 * be created (e.g. due to a name/ID conflict).
	 */
	virtual int createLayer(const std::string& name, int layerID) = 0;

	/**
	 * greebo: Deletes the named layer. All nodes are removed
	 * from this layer before deletion.
	 */
	virtual void deleteLayer(const std::string& name) = 0;

	/**
	 * greebo: Resets the layer system into its ground state. Deletes all
	 * layers except for layer #0 which is renamed to "Default".
	 */
	virtual void reset() = 0;

    typedef std::function<void(int layerId, const std::string& layerName)> LayerVisitFunc;

    /**
     * Functor is called using id and name as arguments
     */
    virtual void foreachLayer(const LayerVisitFunc& visitor) = 0;

	/**
	 * greebo: Returns the ID of the named layer, or -1 if name doesn't exist
	 */
	virtual int getLayerID(const std::string& name) const = 0;

	/**
	 * greebo: Returns the name of the layer with the given ID or "" if it doesn't exist
	 */
	virtual std::string getLayerName(int layerID) const = 0;

	/**
	 * Returns TRUE if the given layer exists, FALSE otherwise.
	 */
	virtual bool layerExists(int layerID) const = 0;

	/**
	 * greebo: Renames the given layer. Returns TRUE on success, FALSE if the name is
	 * already in use.
	 */
	virtual bool renameLayer(int layerID, const std::string& newLayerName) = 0;

	/**
	 * greebo: Returns the ID of the first visible layer or 0 (default layer) if none is visible.
	 */
	virtual int getFirstVisibleLayer() const = 0;

	/**
	 * Returns the ID of the layer that is currently signed as active (will return the default layer
	 * if no other layer is active). Note that the returned layer is not necessarily visible.
	 */
	virtual int getActiveLayer() const = 0;

	/**
	 * Declare the given layer as active. Nothing will happen if the given layer ID is not existent.
	 * This method doesn't change the visibility of the given layer.
	 */
	virtual void setActiveLayer(int layerID) = 0;

	/**
	 * greebo: Queries the visibility of the given layer.
	 */
	virtual bool layerIsVisible(int layerID) = 0;

	/**
	 * greebo: Sets the visibility of the given layer.
	 * This operation will affect all child layers that might be
	 * assigned to this one, recursively.
	 */
	virtual void setLayerVisibility(int layerID, bool visible) = 0;

	/**
	 * greebo: Traverses the selection and adds each node to the given layer.
	 */
	virtual void addSelectionToLayer(int layerID) = 0;

	/**
	 * greebo: Moves all selected nodes to the given layer. This implicitly
	 *         removes the nodes from all other layers.
	 */
	virtual void moveSelectionToLayer(int layerID) = 0;

	/**
	 * greebo: Removes the selected nodes from the given layers.
	 */
	virtual void removeSelectionFromLayer(int layerID) = 0;

	/**
	 * greebo: Updates the visibility of the given node based on the
	 * current layer visibility settings.
	 *
	 * @returns: TRUE if the node was set to "visible", FALSE if the
	 * current layer settings resulted to "invisible" and the
	 * node was therefore hidden.
	 */
	virtual bool updateNodeVisibility(const INodePtr& node) = 0;

	/**
	 * greebo: Sets the selection status of the entire layer.
	 *
	 * @layerID: the ID of the layer to be selected/deselected.
	 * @selected: TRUE if the layer should be selected, FALSE if the members
	 * should be de-selected.
	 */
	virtual void setSelected(int layerID, bool selected) = 0;

    /**
     * Returns the parent layer ID of the layer identified by the given ID.
     * Will return -1 if the given layer doesn't have a parent or doesn't exist.
     */
    virtual int getParentLayer(int layerId) = 0;

    /**
     * Sets the parent of the given child layer, replacing any previous parent.
     *
     * Any layer can be made a child of another layer, as long as the formed
     * tree is staying sane (no recursions).
     * Setting a parent layer ID of -1 will remove the parent and make this
     * a top-level layer.
     *
     * An attempt to form an invalid operation (like modifying the default layer
     * or forming a recursion) will throw a std::runtime_error.
     */
    virtual void setParentLayer(int childLayerId, int parentLayerId) = 0;

    /**
     * Returns true if the given parentLayerId is part of the ancestry of the
     * given candidateLayerId (the node itself is not part of the ancestry).
     *
     * Returns false if any of the given IDs is -1.
     */
    virtual bool layerIsChildOf(int candidateLayerId, int parentLayerId) = 0;

	/**
	 * A signal for client code to get notified about layer creation,
	 * renamings and removal.
	 */
	virtual sigc::signal<void> signal_layersChanged() = 0;

	/**
	 * Fired whenever visibility of a layer has been changed.
	 */
	virtual sigc::signal<void> signal_layerVisibilityChanged() = 0;

    /**
     * Fired whenever a parent of a layer has been changed.
     */
    virtual sigc::signal<void> signal_layerHierarchyChanged() = 0;

	/**
	 * Public signal to get notified about layer membership changes,
	 * e.g. when a node has been added to a layer, or moved to a new one.
	 * Since scene::INodes can be added or removed from layers directly,
	 * without the LayerSystem knowing about this, it is sometimes the
	 * responsibility of that algorithm code to emit this signal itself.
	 */
	virtual sigc::signal<void> signal_nodeMembershipChanged() = 0;
};

class ILayerModule :
	public RegisterableModule
{
public:
    ~ILayerModule() override {}

    /**
     * Creates a new layer manager instance associated to the given scene (root) node
     */
    virtual ILayerManager::Ptr createLayerManager(INode& rootNode) = 0;
};

} // namespace scene

constexpr const char* const MODULE_LAYERS("LayerModule");

inline scene::ILayerModule& GlobalLayerModule()
{
	static module::InstanceReference<scene::ILayerModule> _reference(MODULE_LAYERS);
	return _reference;
}
