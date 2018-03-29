#include "ParticlesManager.h"

#include "editor/ParticleEditorModule.h"
#include "modulesystem/StaticModule.h"

// Static module instances
module::StaticModule<particles::ParticlesManager> particlesManagerModule;
module::StaticModule<ui::ParticleEditorModule> particleEditorModule;
