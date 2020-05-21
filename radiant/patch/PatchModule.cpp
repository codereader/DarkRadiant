#include "PatchCreators.h"
#include "module/StaticModule.h"

// Define the static patchcreator modules
module::StaticModule<patch::Doom3PatchCreator> doom3PatchDef3Creator;
module::StaticModule<patch::Doom3PatchDef2Creator> doom3PatchDef2Creator;
