#include "LayerSystem.h"

#include "stream/textstream.h"
#include "modulesystem/StaticModule.h"

namespace scene {

bool LayerSystem::layerIsVisible(const std::string& layerName) {
	return true;
}

void LayerSystem::addSelectionToLayer(const std::string& layerName) {

}

// RegisterableModule implementation
const std::string& LayerSystem::getName() const {
	static std::string _name(MODULE_LAYERSYSTEM);
	return _name;
}

const StringSet& LayerSystem::getDependencies() const {
	static StringSet _dependencies;
	return _dependencies;
}

void LayerSystem::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "LayerSystem::initialiseModule called.\n";
	
	// Add command targets here
}

void LayerSystem::shutdownModule() {
	
}

// Define the static LayerSystem module
module::StaticModule<LayerSystem> layerSystemModule;

} // namespace scene
