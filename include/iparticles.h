#ifndef IPARTICLES_H_
#define IPARTICLES_H_

#include "generic/constant.h"
#include "modulesystem.h"

#include <boost/function.hpp>

/**
 * Representation of a particles declaration.
 */
class IParticleDef {
public:
	
	/**
	 * Get the name of the particle system.
	 */
	virtual std::string getName() const = 0;

};

/**
 * Callback for evaluation particle defs.
 */
typedef boost::function< void (const IParticleDef&) > ParticleDefVisitor;

/**
 * Abstract interface for the ParticlesManager module.
 */
struct IParticlesManager {
	
	/* Radiant module stuff */
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "particlesmanager");

	/**
	 * Enumerate each particle def.
	 */
	virtual void forEachParticleDef(const ParticleDefVisitor&) const = 0;
	
};

/* Module types */
typedef GlobalModule<IParticlesManager> GlobalParticlesManagerModule;
typedef GlobalModuleRef<IParticlesManager> GlobalParticlesManagerModuleRef;

inline IParticlesManager& GlobalParticlesManager() {
	return GlobalParticlesManagerModule::getTable();	
}

#endif /*IPARTICLES_H_*/
