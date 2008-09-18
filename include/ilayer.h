#ifndef ILAYER_H_
#define ILAYER_H_

#include <set>
#include <string>
#include "imodule.h"

namespace scene {

class INode;
typedef boost::shared_ptr<INode> INodePtr;

// A list of named layers
typedef std::set<int> LayerList;

/** 
 * greebo: Interface of a Layered object.
 */
class Layered
{
public:
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
    virtual LayerList getLayers() const = 0;
};

class ILayerSystem :
	public RegisterableModule
{
public:
	// Visitor class for use with the foreachLayer() method
	class Visitor {
	public:
		virtual void visit(int layerID, std::string layerName) = 0;
	};

	/**
	 * greebo: Creates a new layer with the given name.
	 * 
	 * @returns: the ID of the layer of -1 if the layer could not
	 *           be created (e.g. due to a name conflict).
	 */
	virtual int createLayer(const std::string& name) = 0;

	/**
	 * greebo: Overload: Creates a new layer with the given name and the given ID.
	 * 
	 * @returns: the ID of the layer of -1 if the layer could not
	 *           be created (e.g. due to a name/ID conflict).
	 */
	virtual int createLayer(const std::string& name, int layerID) = 0;

	/**
	 * greebo: Deletes the named layer. All nodes are removed
	 *         from this layer before deletion.
	 */
	virtual void deleteLayer(const std::string& name) = 0;

	/**
	 * greebo: Resets the layer system into its ground state. Deletes all
	 *         layers except for layer #0 which is renamed to "Default".
	 */
	virtual void reset() = 0;

	/**
	 * greebo: Visits each layer using the given visitor.
	 */
	virtual void foreachLayer(Visitor& visitor) = 0;

	/**
	 * greebo: Returns the ID of the named layer, or -1 if name doesn't exist
	 */
	virtual int getLayerID(const std::string& name) const = 0;

	/**
	 * greebo: Returns the name of the layer with the given ID or "" if it doesn't exist
	 */
	virtual std::string getLayerName(int layerID) const = 0;

	/**
	 * greebo: Returns the ID of the first visible layer or -1 if none is visible.
	 */
	virtual int getFirstVisibleLayer() const = 0;

	/**
	 * greebo: Queries the visibility of the given layer.
	 */
	virtual bool layerIsVisible(const std::string& layerName) = 0;
	virtual bool layerIsVisible(int layerID) = 0;

	/**
	 * greebo: Sets the visibility of the given layer.
	 */
	virtual void setLayerVisibility(const std::string& layerName, bool visible) = 0;
	virtual void setLayerVisibility(int layerID, bool visible) = 0;

	/**
	 * greebo: Traverses the selection and adds each node to the given layer.
	 */
	virtual void addSelectionToLayer(const std::string& layerName) = 0;
	virtual void addSelectionToLayer(int layerID) = 0;

	/**
	 * greebo: Moves all selected nodes to the given layer. This implicitly
	 *         removes the nodes from all other layers.
	 */
	virtual void moveSelectionToLayer(const std::string& layerName) = 0;
	virtual void moveSelectionToLayer(int layerID) = 0;

	/**
	 * greebo: Removes the selected nodes from the given layers.
	 */
	virtual void removeSelectionFromLayer(const std::string& layerName) = 0;
	virtual void removeSelectionFromLayer(int layerID) = 0;

	/**
	 * greebo: Updates the visibility of the given node based on the
	 *         current layer visibility settings.
	 *
	 * @returns: TRUE if the node was set to "visible", FALSE if the 
	 *           current layer settings resulted to "invisible" and the 
	 *           node was therefore hidden.
	 */
	virtual bool updateNodeVisibility(const scene::INodePtr& node) = 0;

	/**
	 * greebo: Sets the selection status of the entire layer.
	 * 
	 * @layerID: the ID of the layer to be selected/deselected.
	 * @selected: TRUE if the layer should be selected, FALSE if the members 
	 * should be de-selected.
	 */
	virtual void setSelected(int layerID, bool selected) = 0;
};

} // namespace scene

const std::string MODULE_LAYERSYSTEM("LayerSystem");

inline scene::ILayerSystem& GlobalLayerSystem() {
	// Cache the reference locally
	static scene::ILayerSystem& _layerSystem(
		*boost::static_pointer_cast<scene::ILayerSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_LAYERSYSTEM)
		)
	);
	return _layerSystem;
}

#endif /*ILAYER_H_*/
