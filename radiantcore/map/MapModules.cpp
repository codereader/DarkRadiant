#include "imapformat.h"
#include "module/StaticModule.h"
#include "Map.h"

// Creates the static module instance
module::StaticModuleRegistration<map::Map> staticMapModule;

// Accessor method containing the singleton Map instance
map::Map& GlobalMap()
{
    return *std::static_pointer_cast<map::Map>(
        module::GlobalModuleRegistry().getModule(MODULE_MAP));
}
