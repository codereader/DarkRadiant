#pragma once

#include "irender.h"
#include "iparticlestage.h"

#include "math/AABB.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Matrix4.h"

#include "ParticleQuad.h"
#include "ParticleRenderInfo.h"

namespace particles
{

// Seconds to milliseconds
#define SEC2MS(x) ((x)*1000)
#define MS2SEC(x) ((x)*0.001f)

/// A single bunch of particles, consisting of a renderable set of quads
class RenderableParticleBunch
{
	// The bunch index
	std::size_t _index;

	// The stage this bunch is part of
	const IStageDef& _stage;

	// The quads of this particle bunch
	typedef std::vector<ParticleQuad> Quads;
	Quads _quads;

	// The seed for our local randomiser, as passed by the parent stage
	Rand48::result_type _randSeed;

	// The randomiser itself, which is reset everytime we rebuild the geometry
	Rand48 _random;

	// The flag whether to spawn particles at random locations (standard path calculation)
	bool _distributeParticlesRandomly;

	// Stage-specific offset
	const Vector3& _offset;

	// The matrix to orient quads (owned by the RenderableParticleStage)
	const Matrix4& _viewRotation;

	// The particle direction (instance owned by RenderableParticle)
	const Vector3& _direction;

	// The bounds of this quad group, calculated on demand
	AABB _bounds;

	// The entity colour (instance owned by RenderableParticle)
	const Vector3& _entityColour;

public:
	// Each bunch has a defined zero-based index
	RenderableParticleBunch(std::size_t index,
							Rand48::result_type randSeed,
							const IStageDef& stage,
							const Matrix4& viewRotation,
							const Vector3& direction,
							const Vector3& entityColour);

	std::size_t getIndex() const
	{
		return _index;
	}

	// Update the particle geometry and render information.
	// Time is specified in stage time without offset,in msecs.
	void update(std::size_t time);

    // Add the renderable geometry to the given arrays
    void addVertexData(std::vector<render::RenderVertex>& vertices,
        std::vector<unsigned int>& indices, const Matrix4& localToWorld);

	const AABB& getBounds();

    std::size_t getNumQuads() const
    {
        return _quads.size();
    }

private:
	// Time is measured in seconds!
	float integrate(const IParticleParameter& param, float time)
	{
		return (param.getTo() - param.getFrom()) / _stage.getDuration() * time*time * 0.5f + param.getFrom() * time;
	}

	Vector4 lerpColour(const Vector4& startColour, const Vector4& endColour, float fraction)
	{
		return startColour * (1.0f - fraction) + endColour * fraction;
	}

	void calculateColour(ParticleRenderInfo& particle);

	// Calculates origin at the given time, write result back to the given struct
	void calculateOrigin(ParticleRenderInfo& particle);

	// Handles animFrame stuff, may only be called if animFrames > 0
	void calculateAnim(ParticleRenderInfo& particle);

	// The rotation is used to deviate the offsets should be normalised and not degenerate
	Vector3 getDirection(ParticleRenderInfo& particle, const Matrix4& rotation, const Vector3& distributionOffset);

	Vector3 getDistributionOffset(ParticleRenderInfo& particle, bool distributeParticlesRandomly);

	// Calculates the matrix which rotates faces towards the viewer (used for "aimed" orientation)
	Matrix4 getAimedMatrix(const Vector3& particleVelocity);

	// Handles aimed particles
	void pushAimedParticles(ParticleRenderInfo& particle, std::size_t stageDurationMsec);

	// Generates a new quad using the given struct as data source.
	// colour, s0 and sWidth override the values in info
	void pushQuad(ParticleRenderInfo& particle, const Vector4& colour, float s0 = 0.0f, float sWidth = 1.0f);

	// Makes the quad transition seamless by snapping the adjacent vertices at the midpoint
	void snapQuads(ParticleQuad& curQuad, ParticleQuad& prevQuad);

	void calculateBounds();
};
typedef std::shared_ptr<RenderableParticleBunch> RenderableParticleBunchPtr;

} // namespace
