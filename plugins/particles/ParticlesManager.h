#pragma once

#include "ParticleDef.h"
#include "ParticleStage.h"

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
private:
	// Map of named particle defs
	typedef std::map<std::string, ParticleDefPtr> ParticleDefMap;
	ParticleDefMap _particleDefs;

	typedef std::set<IParticlesManager::Observer*> Observers;
	Observers _observers;

public:
	// IParticlesManager implementation. For documentation see iparticles.h
	void addObserver(IParticlesManager::Observer* observer);
	void removeObserver(IParticlesManager::Observer* observer);
	void forEachParticleDef(const ParticleDefVisitor& visitor) const;
	IParticleDefPtr getParticle(const std::string& name);

	// Finds or creates the particle def with the given name, always returns non-NULL
	ParticleDefPtr findOrInsertParticleDef(const std::string& name);

	// Removes the named particle definition from the storage
	void removeParticleDef(const std::string& name);

	IRenderableParticlePtr getRenderableParticle(const std::string& name);
	void reloadParticleDefs();

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
};
typedef boost::shared_ptr<ParticlesManager> ParticlesManagerPtr;

}
