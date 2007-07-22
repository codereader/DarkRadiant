#include "ParticlesManager.h"

#include "iparticles.h"
#include "ifilesystem.h"
#include "modulesystem/singletonmodule.h"

/*
 * Dependencies class.
 */
class ParticlesManagerDependencies
: public GlobalFileSystemModuleRef
{ };


/* Register with module server */
typedef SingletonModule<particles::ParticlesManager, 
						ParticlesManagerDependencies> ParticlesManagerModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
	// Static module instance
	static ParticlesManagerModule _instance;
	
	// Initialise and register the module	
	initialiseModule(server);
	_instance.selfRegister();
}
