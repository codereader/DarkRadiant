#include "SoundManager.h"

#include "modulesystem/singletonmodule.h"

#include <iostream>


/* Register with module server */

typedef SingletonModule<sound::SoundManager> SoundManagerModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
	// Static module instance
	static SoundManagerModule _instance;
	
	// Initialise and register the module	
	initialiseModule(server);
	_instance.selfRegister();
}
