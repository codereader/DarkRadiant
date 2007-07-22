#ifndef PARTICLESMANAGER_H_
#define PARTICLESMANAGER_H_

#include "iparticles.h"

namespace particles
{

class ParticlesManager
: public IParticlesManager
{
public:
	
	/* Module stuff */
	typedef IParticlesManager Type;
	STRING_CONSTANT(Name, "*");
	
	IParticlesManager* getTable() { 
		return this;
	}
	
	/*
	 * Visit each particles def.
	 */
	void forEachParticleDef(const ParticleDefVisitor& visitor) const;
	
};

}

#endif /*PARTICLESMANAGER_H_*/
