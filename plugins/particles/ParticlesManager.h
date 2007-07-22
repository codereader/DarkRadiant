#ifndef PARTICLESMANAGER_H_
#define PARTICLESMANAGER_H_

#include "ParticleStage.h"

#include "iparticles.h"


/* FORWARD DECLS */
namespace parser { class DefTokeniser; }

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
