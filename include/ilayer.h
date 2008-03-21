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
	virtual bool layerIsVisible(const std::string& layerName) = 0;

	/**
	 * greebo: Updates the visibility of the given node based on the
	 *         current layer visibility settings.
	 *
	 * @returns: TRUE if the node was set to "visible", FALSE if the 
	 *           current layer settings resulted to "invisible" and the 
	 *           node was therefore hidden.
	 */
	virtual bool updateNodeVisibility(const scene::INodePtr& node) = 0;
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
