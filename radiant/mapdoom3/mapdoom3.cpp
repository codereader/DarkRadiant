#include "map/format/Doom3MapFormat.h"
#include "map/format/Doom3PrefabFormat.h"
#include "map/format/Quake4MapFormat.h"
#include "map/format/Quake3MapFormat.h"
#include "map/aas/Doom3AasFileLoader.h"

#include "imapformat.h"
#include "modulesystem/StaticModule.h"

// Static module instances
module::StaticModule<map::Doom3MapFormat> d3MapModule;
module::StaticModule<map::Quake4MapFormat> q4MapModule;
module::StaticModule<map::Doom3PrefabFormat> d3PrefabModule;
module::StaticModule<map::Quake3MapFormat> q3MapModule;
module::StaticModule<map::Doom3AasFileLoader> d3AasModule;
