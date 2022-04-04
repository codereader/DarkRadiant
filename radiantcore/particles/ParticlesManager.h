#pragma once

#include "ParticleDef.h"
#include "StageDef.h"

#include "ParticleLoader.h"
#include "iparticles.h"
#include "parser/DefTokeniser.h"

#include <map>

namespace particles
{

class ParticlesManager :
	public IParticlesManager
{
private:
	ParticleDefMap _particleDefs;

    ParticleLoader _defLoader;

    // Reloaded signal
    sigc::signal<void> _particlesReloadedSignal;

public:
    ParticlesManager();

	// IParticlesManager implementation
    sigc::signal<void>& signal_particlesReloaded() override;

    void forEachParticleDef(const ParticleDefVisitor& visitor) override;

    IParticleDefPtr getDefByName(const std::string& name) override;

	IParticleDefPtr findOrInsertParticleDef(const std::string& name) override;

	void removeParticleDef(const std::string& name) override;

    IRenderableParticlePtr getRenderableParticle(const std::string& name) override;
    IParticleNodePtr createParticleNode(const std::string& name) override;

	void reloadParticleDefs() override;

	void saveParticleDef(const std::string& particle) override;

	// RegisterableModule implementation
	const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;

	static ParticlesManager& Instance()
	{
		return *std::static_pointer_cast<ParticlesManager>(
			module::GlobalModuleRegistry().getModule(MODULE_PARTICLESMANAGER)
		);
	}

private:
    ParticleDefPtr findOrInsertParticleDefInternal(const std::string& name);

    // Since loading is happening in a worker thread, we need to ensure
    // that it's done loading before accessing any defs.
    void ensureDefsLoaded();

    void onParticlesLoaded();
};

}
