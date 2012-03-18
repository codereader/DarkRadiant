#pragma once

#include "ParticleDef.h"
#include "StageDef.h"

#include "iparticles.h"
#include "parser/DefTokeniser.h"

#include <map>

namespace particles
{

/* CONSTANTS */
namespace {
	const char* PARTICLES_DIR = "particles/";
	const char* PARTICLES_EXT = "prt";
}

class ParticlesManager :
	public IParticlesManager
{
	// Map of named particle defs
	typedef std::map<std::string, ParticleDefPtr> ParticleDefMap;
	ParticleDefMap _particleDefs;

    // Reloaded signal
    sigc::signal<void> _particlesReloadedSignal;

public:

	// IParticlesManager implementation
    sigc::signal<void> signal_particlesReloaded() const;
	void forEachParticleDef(const ParticleDefVisitor& visitor) const;
	IParticleDefPtr getDefByName(const std::string& name);
	IParticleNodePtr createParticleNode(const std::string& name);

	// Finds or creates the particle def with the given name, always returns non-NULL
	ParticleDefPtr findOrInsertParticleDef(const std::string& name);

	// Removes the named particle definition from the storage
	void removeParticleDef(const std::string& name);

	IRenderableParticlePtr getRenderableParticle(const std::string& name);
	void reloadParticleDefs();

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

	/**
	 * Accept a stream containing particle definitions to parse and add to the
	 * list.
	 */
	void parseStream(std::istream& s, const std::string& filename);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);

	static ParticlesManager& Instance()
	{
		return *boost::static_pointer_cast<ParticlesManager>(
			module::GlobalModuleRegistry().getModule(MODULE_PARTICLESMANAGER)
		);
	}

private:
	// Recursive-descent parse functions
	void parseParticleDef(parser::DefTokeniser& tok, const std::string& filename);

	static void stripParticleDefFromStream(std::istream& input, std::ostream& output, const std::string& particleName);
};
typedef boost::shared_ptr<ParticlesManager> ParticlesManagerPtr;

}
