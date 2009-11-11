#ifndef IPARTICLES_H_
#define IPARTICLES_H_

#include "imodule.h"
#include <boost/function.hpp>

/**
 * Representation of a particles declaration.
 */
class IParticleDef {
public:
    /**
	 * Destructor
	 */
	virtual ~IParticleDef() {}
	
	/**
	 * Get the name of the particle system.
	 */
	virtual std::string getName() const = 0;

};

/**
 * Callback for evaluation particle defs.
 */
typedef boost::function< void (const IParticleDef&) > ParticleDefVisitor;

const std::string MODULE_PARTICLESMANAGER("ParticlesManager"); 

/**
 * Abstract interface for the ParticlesManager module.
 */
class IParticlesManager :
	public RegisterableModule
{
public:
	/**
	 * Enumerate each particle def.
	 */
	virtual void forEachParticleDef(const ParticleDefVisitor&) const = 0;
};

// Accessor
inline IParticlesManager& GlobalParticlesManager() {
	// Cache the reference locally
	static IParticlesManager& _particlesManager(
		*boost::static_pointer_cast<IParticlesManager>(
			module::GlobalModuleRegistry().getModule(MODULE_PARTICLESMANAGER)
		)
	);
	return _particlesManager;
}

#endif /*IPARTICLES_H_*/
