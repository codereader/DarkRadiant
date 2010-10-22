#ifndef _RENDERABLE_PARTICLE_BUNCH_H_
#define _RENDERABLE_PARTICLE_BUNCH_H_

#include "irender.h"
#include "iparticlestage.h"

#include "math/aabb.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/matrix.h"

#include <boost/random/linear_congruential.hpp>

#include "ParticleQuad.h"

namespace particles
{

// Seconds to milliseconds
#define SEC2MS(x) ((x)*1000)
#define MS2SEC(x) ((x)*0.001f)

class RenderableParticleBunch :
	public OpenGLRenderable
{
private:
	// The bunch index
	std::size_t _index;

	// The stage this bunch is part of
	const IParticleStage& _stage;

	// The quads of this particle bunch
	typedef std::vector<ParticleQuad> Quads;
	Quads _quads;

	// The seed for our local randomiser, as passed by the parent stage
	int _randSeed;

	// The randomiser itself, which is reset everytime we rebuild the geometry
	boost::rand48 _random;

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

public:
	// Each bunch has a defined zero-based index
	RenderableParticleBunch(std::size_t index, 
							int randSeed,
							const IParticleStage& stage,
							const Matrix4& viewRotation,
							const Vector3& direction) :
		_index(index),
		_stage(stage),
		_quads(),
		_randSeed(randSeed),
		_distributeParticlesRandomly(_stage.getRandomDistribution()),
		_offset(_stage.getOffset()),
		_viewRotation(viewRotation),
		_direction(direction)
	{
		// Geometry is written in update(), just reserve the space
	}

	std::size_t getIndex() const
	{
		return _index;
	}

	// Update the particle geometry and render information. 
	// Time is specified in stage time without offset,in msecs.
	void update(std::size_t time);

	void render(const RenderInfo& info) const;

	const AABB& getBounds();

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

	Vector4 getColour(float timeFraction, std::size_t particleIndex);

	// Calculates origin and velocity at the given time, write result back to the given vectors
	void getOriginAndVelocity(float particleTimeSecs, float timeFraction, Vector3& particleOrigin, Vector3& particleVelocity);

	// baseDirection should be normalised and not degenerate
	Vector3 getDirection(const Vector3& baseDirection, const Vector3& distributionOffset);

	Vector3 getDistributionOffset(bool distributeParticlesRandomly);

	// Generates a new quad using the given origin as centroid, angle is in degrees
	void pushQuad(const Vector3& origin, const Vector3& velocity,
				  float size, float aspect, float angle, 
				  const Vector4& colour, float s0 = 0.0f, float sWidth = 1.0f);

	void calculateBounds();
};
typedef boost::shared_ptr<RenderableParticleBunch> RenderableParticleBunchPtr;

} // namespace

#endif /* _RENDERABLE_PARTICLE_BUNCH_H_ */
