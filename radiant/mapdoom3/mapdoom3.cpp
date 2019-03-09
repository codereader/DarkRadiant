#include "Doom3MapFormat.h"
#include "Doom3PrefabFormat.h"
#include "Quake4MapFormat.h"
#include "Quake3MapFormat.h"
#include "aas/Doom3AasFileLoader.h"

#include "imapformat.h"
#include "modulesystem/StaticModule.h"

// Static module instances
module::StaticModule<map::Doom3MapFormat> d3MapModule;
module::StaticModule<map::Quake4MapFormat> q4MapModule;
module::StaticModule<map::Doom3PrefabFormat> d3PrefabModule;
module::StaticModule<map::Quake3MapFormat> q3MapModule;
module::StaticModule<map::Doom3AasFileLoader> d3AasModule;
