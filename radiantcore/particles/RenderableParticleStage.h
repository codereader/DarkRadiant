#pragma once

#include "RenderableParticleBunch.h"
#include "render/RenderableGeometry.h"

namespace particles
{

/**
 * \brief
 * Renderable object drawing the particles defined by a StageDef.
 *
 * Each particle stage generates its geometry in one or more cycles.  Each cycle
 * comes as a bunch of quads with a defined lifespan. It's possible for quads of
 * one cycle to exist during the lifetime of the next cycle (if bunching is set
 * to values below 1), but there can always be 2 bunches active at the same
 * time.
 */
class RenderableParticleStage :
    public render::RenderableGeometry
{
	// The stage def we're rendering
	const IStageDef& _stageDef;

	// We use these values as seeds whenever we instantiate a new bunch of particles
	// each bunch has a distinct index and is using the same seed during the lifetime
	// of this particle stage
	std::size_t _numSeeds;
	std::vector<Rand48::result_type> _seeds;

	std::vector<RenderableParticleBunchPtr> _bunches;

	// The rotation matrix to orient particles
	Matrix4 _viewRotation;
    // Matrix to produce world coordinates
	Matrix4 _localToWorld;

	// The particle direction (instance owned by RenderableParticle)
	const Vector3& _direction;

	// The bounds of this stage (calculated on demand)
	AABB _bounds;

	// The entity colour (instance owned by RenderableParticle)
	const Vector3& _entityColour;

public:
	RenderableParticleStage(const IStageDef& stage, 
							Rand48& random, 
							const Vector3& direction,
							const Vector3& entityColour);

	// Generate particle geometry, time is absolute in msecs
	void update(std::size_t time, const Matrix4& viewRotation);

    void submitGeometry(const ShaderPtr& shader, const Matrix4& localToWorld);

	const AABB& getBounds();

    /// Return the stage definition associated with this renderable
	const IStageDef& getDef() const;

protected:
    void updateGeometry() override;

private:
	// Returns the correct rotation matrix required by the stage orientation settings
	void calculateStageViewRotation(const Matrix4& viewRotation);

	void ensureBunches(std::size_t localTimeMSec);

	RenderableParticleBunchPtr createBunch(std::size_t cycleIndex);

	Rand48::result_type getSeed(std::size_t cycleIndex);

	RenderableParticleBunchPtr getExistingBunchByIndex(std::size_t index);

	void calculateBounds();
};
typedef std::shared_ptr<RenderableParticleStage> RenderableParticleStagePtr;

} // namespace
