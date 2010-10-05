#ifndef IPARTICLES_H_
#define IPARTICLES_H_

#include "imodule.h"
#include <boost/function.hpp>

#include "irenderable.h"

class RenderSystem;

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
typedef boost::shared_ptr<IParticleDef> IParticleDefPtr;

/**
 * A renderable particle, which is capable of compiling the
 * particle system into actual geometry usable for the backend rendersystem.
 *
 * As it derives from Renderable, this object can be added to a RenderableCollector
 * during the front-end render phase.
 */
class IRenderableParticle :
	public Renderable
{
public:
	/**
	 * Update the particle geometry using the given time in milliseconds.
	 * The rendersystem is needed for acquiring the shaders.
	 */
	virtual void update(std::size_t time, RenderSystem& renderSystem) = 0;

	/**
	 * Get the particle definition used by this renderable.
	 */
	virtual const IParticleDefPtr& getParticleDef() const = 0;

	/**
	 * Set the particle definition. You'll need to call update() after
	 * setting a new particle def.
	 */
	virtual void setParticleDef(const IParticleDefPtr& def) = 0;
};
typedef boost::shared_ptr<IRenderableParticle> IRenderableParticlePtr;

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

	/**
	 * Get a named particle definition, returns NULL if not found.
	 */
	virtual IParticleDefPtr getParticle(const std::string& name) = 0;

	/**
	 * Create a renderable particle, which is capable of compiling the
	 * particle system into actual geometry usable for the backend rendersystem.
	 * 
	 * @returns: the renderable particle instance or NULL if the named particle was not found.
	 */
	virtual IRenderableParticlePtr getRenderableParticle(const std::string& name) = 0;
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
