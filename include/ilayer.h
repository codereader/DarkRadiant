#ifndef ILAYER_H_
#define ILAYER_H_

#include <set>
#include <string>
#include "imodule.h"

namespace scene {

// A list of named layers
typedef std::set<std::string> LayerList;

/** 
 * greebo: Interface of a Layered object.
 */
class Layered
{
public:
	/**
     * Add this object to the named layer.
     */
    virtual void addToLayer(const std::string& layer) = 0;

    /**
     * Remove this object from the named layer.
     */
    virtual void removeFromLayer(const std::string& layer) = 0;

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
