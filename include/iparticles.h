#ifndef IPARTICLES_H_
#define IPARTICLES_H_

#include "imodule.h"
#include <boost/function.hpp>

namespace particles
{

// see iparticlestage.h for definition
class IParticleStage;

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

	/**
	 * Set/get the depth hack flag
	 */
	virtual float getDepthHack() const = 0;
	virtual void setDepthHack(float value) = 0;

	/**
	 * Returns the number of stages for this particle system.
	 */
	virtual std::size_t getNumStages() = 0;

	/**
	 * Return a specific particle stage (const version)
	 */
	virtual const IParticleStage& getParticleStage(std::size_t stageNum) const = 0;

	/**
	 * Return a specific particle stage (non-const version)
	 */
	virtual IParticleStage& getParticleStages(std::size_t stageNum) = 0;
};

/**
 * Callback for evaluation particle defs.
 */
typedef boost::function< void (const IParticleDef&) > ParticleDefVisitor;

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

} // namespace

const char* const MODULE_PARTICLESMANAGER = "ParticlesManager";

// Accessor
inline particles::IParticlesManager& GlobalParticlesManager()
{
	// Cache the reference locally
	static particles::IParticlesManager& _particlesManager(
		*boost::static_pointer_cast<particles::IParticlesManager>(
			module::GlobalModuleRegistry().getModule(MODULE_PARTICLESMANAGER)
		)
	);
	return _particlesManager;
}

#endif /*IPARTICLES_H_*/
