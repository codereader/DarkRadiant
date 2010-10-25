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

	std::string _debugInfo;

	// A structure holding info about how to draw a certain
	// particle, including texcoords, fade colour, etc.
	// This info can apply to a single quad or a quad group 
	// if the particle stage is animated or aimed
	struct ParticleInfo
	{
		std::size_t index;	// zero-based index of this particle within a stage

		float timeSecs;		// time in seconds
		float timeFraction;	// time fraction within particle lifetime

		Vector3 origin;
		Vector3 velocity;	// velocity of this quad/particle
		Vector4 colour;		// resulting colour

		float angle;		// the angle of the quad
		float size;			// the desired size (might be overridden when aimed)
		float aspect;		// the desired aspect ratio (might be overridden when aimed)

		float sWidth;		// the horizontal amount of texture space occupied by this particle (for anims)
		float t0;			// Vertical texture coordinate
		float tWidth;		// the vertical amount of texture space occupied by this particle (for aiming)

		float rand[4];		// 4 random numbers needed for pathing

		std::size_t animFrames; // animation: number of frames (0 if not animated)
		std::size_t curFrame;	// animation: current frame
		std::size_t nextFrame;	// animation: next frame

		Vector4 curColour;
		Vector4 nextColour;

		ParticleInfo() :
			angle(0),
			sWidth(1),
			t0(0),
			tWidth(1)
		{}
	};

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

	std::string getDebugInfo();

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

	void calculateColour(ParticleInfo& particle);

	// Calculates origin and velocity at the given time, write result back to the given struct
	void calculateOriginAndVelocity(ParticleInfo& particle);

	// Handles animFrame stuff, may only be called if animFrames > 0
	void calculateAnim(ParticleInfo& particle);

	// baseDirection should be normalised and not degenerate
	Vector3 getDirection(ParticleInfo& particle, const Vector3& baseDirection, const Vector3& distributionOffset);

	Vector3 getDistributionOffset(bool distributeParticlesRandomly);

	// Calculates the matrix which rotates faces towards the viewer (used for "aimed" orientation)
	Matrix4 getAimedMatrix(const Vector3& particleVelocity);

	// Handles aimed particles
	void pushAimedParticles(ParticleInfo& particle, std::size_t stageDurationMsec);

	// Generates a new quad using the given struct as data source. 
	// colour, s0 and sWidth override the values in info
	void pushQuad(ParticleInfo& particle, const Vector4& colour, float s0 = 0.0f, float sWidth = 1.0f);

	// Makes the quad transition seamless by snapping the adjacent vertices at the midpoint
	void snapQuads(ParticleQuad& curQuad, ParticleQuad& prevQuad);

	void calculateBounds();
};
typedef boost::shared_ptr<RenderableParticleBunch> RenderableParticleBunchPtr;

} // namespace

#endif /* _RENDERABLE_PARTICLE_BUNCH_H_ */
