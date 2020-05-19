#pragma once

#include "ParticleDef.h"
#include "StageDef.h"

#include "ThreadedDefLoader.h"
#include "iparticles.h"
#include "parser/DefTokeniser.h"

#include <map>

namespace particles
{

class ParticlesManager :
	public IParticlesManager
{
	// Map of named particle defs
	typedef std::map<std::string, ParticleDefPtr> ParticleDefMap;

	ParticleDefMap _particleDefs;

    util::ThreadedDefLoader<void> _defLoader;

    // Reloaded signal
    sigc::signal<void> _particlesReloadedSignal;

public:
    ParticlesManager();

	// IParticlesManager implementation
    sigc::signal<void> signal_particlesReloaded() const override;

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
    void initialiseModule(const ApplicationContext& ctx) override;

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

    /**
    * Accept a stream containing particle definitions to parse and add to the
    * list.
    */
    void parseStream(std::istream& s, const std::string& filename);

	// Recursive-descent parse functions
	void parseParticleDef(parser::DefTokeniser& tok, const std::string& filename);

	static void stripParticleDefFromStream(std::istream& input, std::ostream& output, const std::string& particleName);
};
typedef std::shared_ptr<ParticlesManager> ParticlesManagerPtr;

}
