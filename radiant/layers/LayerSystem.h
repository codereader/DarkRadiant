#ifndef LAYERSYSTEM_H_
#define LAYERSYSTEM_H_

#include "ilayer.h"

namespace scene {

class LayerSystem :
	public ILayerSystem
{
public:
	bool layerIsVisible(const std::string& layerName);

	void addSelectionToLayer(const std::string& layerName);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();
};

} // namespace scene

#endif /* LAYERSYSTEM_H_ */
