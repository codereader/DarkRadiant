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

class ParticlesManager
: public IParticlesManager
{
	// Map of named particle defs
	typedef std::map<std::string, ParticleDef> ParticleDefMap;
	ParticleDefMap _particleDefs;
	
private:
	
	// Recursive-descent parse functions
	void parseParticleDef(parser::DefTokeniser& tok);
	ParticleStage parseParticleStage(parser::DefTokeniser& tok);
	
public:
	
	/* Module stuff */
	typedef IParticlesManager Type;
	STRING_CONSTANT(Name, "*");
	
	IParticlesManager* getTable() { 
		return this;
	}
	
	/*
	 * Main constructor.
	 */
	ParticlesManager();
	
	/*
	 * Visit each particles def.
	 */
	void forEachParticleDef(const ParticleDefVisitor& visitor) const;
	
	/**
	 * Accept a string containing particle definitions to parse and add to the
	 * list.
	 */
	void parseString(const std::string& s);
	
};

}

#endif /*PARTICLESMANAGER_H_*/
