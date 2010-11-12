#ifndef PARTICLESMANAGER_H_
#define PARTICLESMANAGER_H_

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
	IRenderableParticlePtr getRenderableParticle(const std::string& name);
	void reloadParticleDefs();

	/**
	 * Accept a stream containing particle definitions to parse and add to the
	 * list.
	 */
	void parseStream(std::istream& s);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);

private:
	// Recursive-descent parse functions
	void parseParticleDef(parser::DefTokeniser& tok);

	// Finds or creates the particle def with the given name, always returns non-NULL
	ParticleDefPtr findOrInsertParticleDef(const std::string& name);
};
typedef boost::shared_ptr<ParticlesManager> ParticlesManagerPtr;

}

#endif /*PARTICLESMANAGER_H_*/
