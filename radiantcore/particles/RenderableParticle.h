#pragma once

#include "RenderableParticleStage.h"

#include "iparticles.h"
#include "irender.h"

#include "math/AABB.h"
#include "render.h"
#include <map>
#include <sigc++/connection.h>

namespace particles
{

/// Implementation of IRenderableParticle
class RenderableParticle : public IRenderableParticle,
                           public sigc::trackable
{
	// The particle definition containing the stage info
	IParticleDef::Ptr _particleDef;

    // Signal connection from the particle def
    sigc::connection _defConnection;

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
	Rand48 _random;

	// The particle direction, usually set by the emitter entity or the preview
	Vector3 _direction;

	// Holds the bounds of all stages at the current time. Will be updated
	// by calls to getBounds(), otherwise might hold outdated bounds information.
	AABB _bounds;

	// The colour used when "use entity colour" is activated.
	Vector3 _entityColour;

	// The associated rendersystem, needed to get time an shaders
	RenderSystemWeakPtr _renderSystem;

public:
	RenderableParticle(const IParticleDef::Ptr& particleDef);

	~RenderableParticle();

    void clearRenderables();

	// Time is in msecs
	void update(const Matrix4& viewRotation, const Matrix4& localToWorld, IRenderEntity* entity) override;

    void onPreRender(const VolumeTest& volume) override;
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;

	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // never highlighted
	}

	const IParticleDef::Ptr& getParticleDef() const override;
	void setParticleDef(const IParticleDef::Ptr& def) override;

	void setMainDirection(const Vector3& direction) override;
	void setEntityColour(const Vector3& colour) override;

	// Updates bounds from stages and returns the value
	const AABB& getBounds() override;

private:
	void calculateBounds();

	// Sort stages into groups sharing a material, without capturing the shader yet
	void setupStages();

	// Capture all shaders, if necessary
	void ensureShaders(RenderSystem& renderSystem);
};
typedef std::shared_ptr<RenderableParticle> RenderableParticlePtr;

} // namespace
