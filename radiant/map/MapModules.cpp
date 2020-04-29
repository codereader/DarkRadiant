#include "format/portable/PortableMapFormat.h"
#include "format/Doom3MapFormat.h"
#include "format/Doom3PrefabFormat.h"
#include "format/Quake4MapFormat.h"
#include "format/Quake3MapFormat.h"
#include "aas/Doom3AasFileLoader.h"

#include "imapformat.h"
#include "modulesystem/StaticModule.h"
#include "Map.h"

// Static module instances
module::StaticModule<map::format::PortableMapFormat> portableMapModule;
module::StaticModule<map::Doom3MapFormat> d3MapModule;
module::StaticModule<map::Quake4MapFormat> q4MapModule;
module::StaticModule<map::Doom3PrefabFormat> d3PrefabModule;
module::StaticModule<map::Quake3MapFormat> q3MapModule;
module::StaticModule<map::Doom3AasFileLoader> d3AasModule;

// Creates the static module instance
module::StaticModule<map::Map> staticMapModule;

// Accessor method containing the singleton Map instance
map::Map& GlobalMap()
{
    return *std::static_pointer_cast<map::Map>(
        module::GlobalModuleRegistry().getModule(MODULE_MAP));
}
