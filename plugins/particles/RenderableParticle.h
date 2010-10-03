#ifndef _RENDERABLE_PARTICLE_H_
#define _RENDERABLE_PARTICLE_H_

#include "iparticles.h"
#include "irender.h"

namespace particles
{

class RenderableParticle :
	public IRenderableParticle,
	public OpenGLRenderable
{
private:
	// The particle definition containing the stage info
	IParticleDefPtr _particleDef;

public:
	RenderableParticle(const IParticleDefPtr& particleDef) :
		_particleDef(particleDef)
	{}

	void update(std::size_t time)
	{
		// TODO: Evaluate particle state
		// TODO: Update renderable geometry
		// TODO: Capture shaders
	}

	// Front-end render methods
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const 
	{
		// TODO
	}

	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const 
	{
		// TODO
	}

	// OpenGLRenderable implementation
	void render(const RenderInfo& info) const
	{
		// TODO
	}

	/**
	 * Get the particle definition used by this renderable.
	 */
	const IParticleDefPtr& getParticleDef() const
	{
		return _particleDef;
	}

	/**
	 * Set the particle definition (triggers an update(0) call).
	 */
	void setParticleDef(const IParticleDefPtr& def) 
	{
		_particleDef = def;
		update(0);
	}
};
typedef boost::shared_ptr<RenderableParticle> RenderableParticlePtr;

} // namespace

#endif /* _RENDERABLE_PARTICLE_H_ */
