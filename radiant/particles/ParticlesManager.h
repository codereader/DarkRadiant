#pragma once

#include "ParticleDef.h"
#include "StageDef.h"

#include "ThreadedDefLoader.h"
#include "iparticles.h"
#include "parser/DefTokeniser.h"

#include <map>

namespace particles
{

/* CONSTANTS */
namespace 
{
	const char* PARTICLES_DIR = "particles/";
	const char* PARTICLES_EXT = "prt";
}

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

	// Finds or creates the particle def with the given name, always returns non-NULL
	ParticleDefPtr findOrInsertParticleDef(const std::string& name);

	// Removes the named particle definition from the storage
	void removeParticleDef(const std::string& name);

    IRenderableParticlePtr getRenderableParticle(const std::string& name) override;
    IParticleNodePtr createParticleNode(const std::string& name) override;

	void reloadParticleDefs() override;

	/**
	 * Writes the named particle declaration to the file it is associated with, 
	 * replacing any existing declaration with the same name in that file.
	 * Any other particle declarations that happen to be declared in the same file
	 * will be preserved.
	 *
	 * If the associated file is stored in a PK4, a copy is written to the current
	 * fs_game ("mod") folder and that file is used for the save operation. 
	 * No other particle declaration will be removed from the copied file.
	 *
	 * Note: this method does not check if the named particle system is already
	 * defined in a different file.
	 *
	 * throws a std::runtime_error on any failure.
	 */
	void saveParticleDef(const std::string& particle);

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
