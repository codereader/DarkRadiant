#include "ParticlesManager.h"

#include "iparticles.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "debugging/debugging.h"

#include "editor/ParticleEditorModule.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	registry.registerModule(particles::ParticlesManagerPtr(new particles::ParticlesManager));
	registry.registerModule(ui::ParticleEditorModulePtr(new ui::ParticleEditorModule));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
