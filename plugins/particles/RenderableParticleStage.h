#ifndef _RENDERABLE_PARTICLE_STAGE_H_
#define _RENDERABLE_PARTICLE_STAGE_H_

#include "RenderableParticleBunch.h"

namespace particles
{

/**
 * greebo: Each particle stage generates its geometry in one or more cycles.
 * Each cycle comes as a bunch of quads with a defined lifespan. It's possible
 * for quads of one cycle to exist during the lifetime of the next cycle (if bunching 
 * is set to values below 1), but there can always be 2 bunches active at the same time.
 */
class RenderableParticleStage :
	public OpenGLRenderable
{
private:
	// The stage def we're rendering
	const IParticleStage& _stage;

	// We use these values as seeds whenever we instantiate a new bunch of particles
	// each bunch has a distinct index and is using the same seed during the lifetime
	// of this particle stage
	std::size_t _numSeeds;
	std::vector<int> _seeds;

	std::vector<RenderableParticleBunchPtr> _bunches;

	// The rotation matrix to orient particles
	Matrix4 _viewRotation;

	// The particle direction (instance owned by RenderableParticle)
	const Vector3& _direction;

	// The bounds of this stage (calculated on demand)
	AABB _bounds;

public:
	RenderableParticleStage(const IParticleStage& stage, boost::rand48& random, const Vector3& direction);

	void render(const RenderInfo& info) const;

	// Generate particle geometry, time is absolute in msecs 
	void update(std::size_t time, const Matrix4& viewRotation);

	const AABB& getBounds();

	std::string getDebugInfo();

private:
	// Returns the correct rotation matrix required by the stage orientation settings
	void calculateStageViewRotation(const Matrix4& viewRotation);

	void ensureBunches(std::size_t localTimeMSec);

	RenderableParticleBunchPtr createBunch(std::size_t cycleIndex);

	int getSeed(std::size_t cycleIndex);

	RenderableParticleBunchPtr getExistingBunchByIndex(std::size_t index);

	void calculateBounds();
};
typedef boost::shared_ptr<RenderableParticleStage> RenderableParticleStagePtr;

} // namespace

#endif /* _RENDERABLE_PARTICLE_STAGE_H_ */
