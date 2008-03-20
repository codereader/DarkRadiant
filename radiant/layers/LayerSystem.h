#ifndef LAYERSYSTEM_H_
#define LAYERSYSTEM_H_

#include "ilayer.h"

namespace scene {

class LayerSystem {

public:
	bool layerIsVisible(const std::string& layerName);
};

} // namespace scene

// Global accessor
scene::LayerSystem& GlobalLayerSystem();

#endif /* LAYERSYSTEM_H_ */
