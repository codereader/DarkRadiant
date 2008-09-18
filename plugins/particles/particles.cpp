#include "ParticlesManager.h"

#include "iparticles.h"
#include "ifilesystem.h"
#include "itextstream.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(particles::ParticlesManagerPtr(new particles::ParticlesManager));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
