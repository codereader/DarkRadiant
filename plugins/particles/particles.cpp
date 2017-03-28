#include "ParticlesManager.h"

#include "iparticles.h"
#include "ifilesystem.h"

#include "editor/ParticleEditorModule.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(particles::ParticlesManagerPtr(new particles::ParticlesManager));
	registry.registerModule(ui::ParticleEditorModulePtr(new ui::ParticleEditorModule));
}
