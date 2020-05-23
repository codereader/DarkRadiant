#include "aas/Doom3AasFileLoader.h"

#include "imapformat.h"
#include "module/StaticModule.h"
#include "Map.h"

// Static module instances
module::StaticModule<map::Doom3AasFileLoader> d3AasModule;

// Creates the static module instance
module::StaticModule<map::Map> staticMapModule;

// Accessor method containing the singleton Map instance
map::Map& GlobalMap()
{
    return *std::static_pointer_cast<map::Map>(
        module::GlobalModuleRegistry().getModule(MODULE_MAP));
}
