#ifndef _RENDERABLE_PARTICLE_H_
#define _RENDERABLE_PARTICLE_H_

#include "RenderableParticleStage.h"

#include "iparticles.h"
#include "irender.h"

#include "math/AABB.h"
#include "render.h"
#include <map>
#include <boost/random/linear_congruential.hpp>

namespace particles
{

class RenderableParticle :
	public IRenderableParticle,
	public IParticleDef::Observer
{
private:
	// The particle definition containing the stage info
	IParticleDefPtr _particleDef;

	typedef std::vector<RenderableParticleStagePtr> RenderableParticleStageList;

	// Particle stages using the same shader get grouped
	struct ParticleStageGroup
	{
		ShaderPtr shader;
		RenderableParticleStageList stages;
	};

	// Each captured shader can have one or more particle stages assigned to it
	typedef std::map<std::string, ParticleStageGroup> ShaderMap;
	ShaderMap _shaderMap;

	// The random number generator, this is used to generate "constant"
	// starting values for each bunch of particles. This enables us
	// to go back in time when rendering the particle stage.
	boost::rand48 _random;

	// The particle direction, usually set by the emitter entity or the preview
	Vector3 _direction;

	// Holds the bounds of all stages at the current time. Will be updated
	// by calls to getBounds(), otherwise might hold outdated bounds information.
	AABB _bounds;

	// The colour used when "use entity colour" is activated.
	Vector3 _entityColour;

public:
	RenderableParticle(const IParticleDefPtr& particleDef);

	~RenderableParticle();

	// Time is in msecs
	void update(std::size_t time, RenderSystem& renderSystem, const Matrix4& viewRotation);

	// Front-end render methods
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume, 
					 const Matrix4& localToWorld, const IRenderEntity* entity) const;

	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume, 
						 const Matrix4& localToWorld, const IRenderEntity* entity) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		// TODO: Remove argument from update() and use the parameter passed here
	}

	bool isHighlighted() const
	{
		return false; // never highlighted
	}

	const IParticleDefPtr& getParticleDef() const;
	void setParticleDef(const IParticleDefPtr& def);

	void setMainDirection(const Vector3& direction);
	void setEntityColour(const Vector3& colour);

	// Updates bounds from stages and returns the value
	const AABB& getBounds();

	// IParticleDef::Observer implementation
	void onParticleReload();
	void onParticleStageOrderChanged();
	void onParticleStageAdded();
	void onParticleStageRemoved();
	void onParticleStageChanged();
	void onParticleStageMaterialChanged();

private:
	void calculateBounds();

	// Sort stages into groups sharing a material, without capturing the shader yet
	void setupStages();

	// Capture all shaders, if necessary
	void ensureShaders(RenderSystem& renderSystem);
};
typedef boost::shared_ptr<RenderableParticle> RenderableParticlePtr;

} // namespace

#endif /* _RENDERABLE_PARTICLE_H_ */
