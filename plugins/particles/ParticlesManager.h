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
	// Map of named particle defs
	typedef std::map<std::string, ParticleDefPtr> ParticleDefMap;
	ParticleDefMap _particleDefs;
	
private:
	
	// Recursive-descent parse functions
	void parseParticleDef(parser::DefTokeniser& tok);
	
public:
	/*
	 * Visit each particles def.
	 */
	void forEachParticleDef(const ParticleDefVisitor& visitor) const;

	/**
	 * Get a named particle definition, returns NULL if not found.
	 */
	IParticleDefPtr getParticle(const std::string& name);

	/**
	 * Create a renderable particle, which is capable of compiling the
	 * particle system into actual geometry usable for the backend rendersystem.
	 * 
	 * @returns: the renderable particle instance or NULL if the named particle was not found.
	 */
	IRenderableParticlePtr getRenderableParticle(const std::string& name);
	
	/**
	 * Accept a stream containing particle definitions to parse and add to the
	 * list.
	 */
	void parseStream(std::istream& s);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<ParticlesManager> ParticlesManagerPtr;

}

#endif /*PARTICLESMANAGER_H_*/
