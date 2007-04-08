#include "SoundManager.h"

#include "ifilesystem.h"
#include "modulesystem/singletonmodule.h"

/*
 * Dependencies class.
 */
class SoundManagerDependencies
: public GlobalFileSystemModuleRef
{ };


/* Register with module server */

typedef SingletonModule<sound::SoundManager, 
						SoundManagerDependencies> SoundManagerModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
	// Static module instance
	static SoundManagerModule _instance;
	
	// Initialise and register the module	
	initialiseModule(server);
	_instance.selfRegister();
}
