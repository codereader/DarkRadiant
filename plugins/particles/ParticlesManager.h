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
	typedef std::map<std::string, ParticleDef> ParticleDefMap;
	ParticleDefMap _particleDefs;
	
private:
	
	// Recursive-descent parse functions
	void parseParticleDef(parser::DefTokeniser& tok);
	ParticleStage parseParticleStage(parser::DefTokeniser& tok);
	
public:
	/*
	 * Visit each particles def.
	 */
	void forEachParticleDef(const ParticleDefVisitor& visitor) const;
	
	/**
	 * Accept a stream containing particle definitions to parse and add to the
	 * list.
	 */
	void parseStream(std::istream& s);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<ParticlesManager> ParticlesManagerPtr;

}

#endif /*PARTICLESMANAGER_H_*/
