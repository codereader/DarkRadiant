#ifndef LAYERSYSTEM_H_
#define LAYERSYSTEM_H_

#include <vector>
#include "ilayer.h"
#include "generic/callback.h"

namespace scene {

class LayerSystem :
	public ILayerSystem
{
	// greebo: An array of booleans reflects the visibility status
	// of all layers. Indexed by the layer id, it can be used to
	// quickly check whether a layer is visible or not.
	typedef std::vector<bool> LayerVisibilityList;
	LayerVisibilityList _layerVisibility;

public:
	LayerSystem();

	virtual bool layerIsVisible(const std::string& layerName);

	virtual void setLayerVisibility(const std::string& layerName, bool visible);

	void addSelectionToLayer(const std::string& layerName);

	virtual bool updateNodeVisibility(const scene::INodePtr& node);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

	void toggleLayerVisibility();
	typedef MemberCaller<LayerSystem, &LayerSystem::toggleLayerVisibility> ToggleCaller;

	void addSelectionToLayer1();
	typedef MemberCaller<LayerSystem, &LayerSystem::addSelectionToLayer1> AddSelectionCaller;

private:
	// Internal callback, updates the scenegraph
	void layerVisibilityChanged();
};

} // namespace scene

#endif /* LAYERSYSTEM_H_ */
