#include "PatchCreators.h"
#include "modulesystem/StaticModule.h"

// Define the static patchcreator modules
module::StaticModule<Doom3PatchCreator> doom3PatchDef3Creator;
module::StaticModule<Doom3PatchDef2Creator> doom3PatchDef2Creator;
