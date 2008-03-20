#include "LayerSystem.h"

namespace scene {

bool LayerSystem::layerIsVisible(const std::string& layerName) {
	return true;
}

} // namespace scene

scene::LayerSystem& GlobalLayerSystem() {
	static scene::LayerSystem _layerSystem;
	return _layerSystem;
}
