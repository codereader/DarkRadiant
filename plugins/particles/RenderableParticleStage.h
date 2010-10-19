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
	RenderableParticleStage(const IParticleStage& stage, boost::rand48& random, const Vector3& direction) :
		_stage(stage),
		_numSeeds(32),
		_seeds(_numSeeds),
		_bunches(2), // two bunches
		_viewRotation(Matrix4::getIdentity()), // is re-calculated each update anyway
		_direction(direction)
	{
		// Generate our vector of random numbers used seed particle bunches
		// using the random number generator as provided by our parent particle system
		for (std::size_t i = 0; i < _numSeeds; ++i)
		{
			_seeds[i] = random();
		}
	}

	void render(const RenderInfo& info) const
	{
		// Draw up to two active bunches
		if (_bunches[0])
		{
			_bunches[0]->render(info);
		}
		
		if (_bunches[1])
		{
			_bunches[1]->render(info);
		}
	}

	// Generate particle geometry, time is absolute in msecs 
	void update(std::size_t time, const Matrix4& viewRotation)
	{
		// Invalidate our bounds information
		_bounds = AABB();

		// Check time offset (msecs)
		std::size_t timeOffset = static_cast<std::size_t>(SEC2MS(_stage.getTimeOffset()));

		if (time < timeOffset)
		{
			// We're still in the timeoffset zone where particle spawn is inhibited
			_bunches[0].reset();
			_bunches[1].reset();
			return;
		}

		// Time >= timeOffset at this point

		// Get rid of the time offset
		std::size_t localtimeMsec = time - timeOffset;

		// Consider stage orientation (x,y,z,view,aimed)
		calculateStageViewRotation(viewRotation);

		// Make sure the correct bunches are allocated for this stage time
		ensureBunches(localtimeMsec);

		// The 0 bunch is the active one, the 1 bunch is the previous one if not null

		// Tell the particle batches to update their geometry
		if (_bunches[0] != NULL)
		{
			// Get one of our seed values
			_bunches[0]->update(localtimeMsec);
		}

		if (_bunches[1] != NULL)
		{
			_bunches[1]->update(localtimeMsec);
		}
	}

	const AABB& getBounds()
	{
		if (!_bounds.isValid())
		{
			calculateBounds();
		}

		return _bounds;
	}

private:
	// Returns the correct rotation matrix required by the stage orientation settings
	void calculateStageViewRotation(const Matrix4& viewRotation)
	{
		switch (_stage.getOrientationType())
		{
		case IParticleStage::ORIENTATION_VIEW:
			_viewRotation = viewRotation;
			break;

		case IParticleStage::ORIENTATION_X:
			// Rotate the z vector such that it faces the x axis, and use that as transform
			_viewRotation = Matrix4::getRotation(Vector3(0,0,1), Vector3(1,0,0));
			break;

		case IParticleStage::ORIENTATION_Y:
			// Rotate the z vector such that it faces the y axis, and use that as transform
			_viewRotation = Matrix4::getRotation(Vector3(0,0,1), Vector3(0,1,0));
			break;

		case IParticleStage::ORIENTATION_Z:
			// Particles are already facing the z axis by default
			_viewRotation = Matrix4::getIdentity();
			break;

		default:
			_viewRotation = Matrix4::getIdentity();
		};
	}

	void ensureBunches(std::size_t localTimeMSec)
	{
		// Check which bunches is active at this time
		float cycleFrac = floor(static_cast<float>(localTimeMSec) / _stage.getCycleMsec());

		std::size_t curCycleIndex = static_cast<std::size_t>(cycleFrac);

		if (curCycleIndex == 0)
		{
			// This is the only active bunch (the first one), there is no previous cycle
			// it's possible that this one is already existing.
			if (_bunches[0] == NULL || _bunches[0]->getIndex() != curCycleIndex)
			{
				// First bunch is not matching, re-assign
				_bunches[0] = createBunch(curCycleIndex);
			}

			// Reset the previous bunch in any case
			_bunches[1].reset();
		}
		else
		{
			// Current cycle > 0, this means we have possibly two active ones
			std::size_t prevCycleIndex = curCycleIndex - 1;

			// Reuse any existing instances, to avoid re-instancing them all over again
			RenderableParticleBunchPtr cur = getExistingBunchByIndex(curCycleIndex);
			RenderableParticleBunchPtr prev = getExistingBunchByIndex(prevCycleIndex);

			std::size_t numCycles = static_cast<std::size_t>(_stage.getCycles());

			if (numCycles > 0 && curCycleIndex > numCycles)
			{
				// We've exceeded the maximum number of cycles
				_bunches[0].reset();
			}
			else if (cur != NULL)
			{
				_bunches[0] = cur;
			}
			else
			{
				_bunches[0] = createBunch(curCycleIndex);
			}

			if (numCycles > 0 && prevCycleIndex > numCycles)
			{
				// We've exceeded the maximum number of cycles
				_bunches[1].reset();
			}
			else if (prev != NULL)
			{
				_bunches[1] = prev;
			}
			else
			{
				_bunches[1] = createBunch(prevCycleIndex);
			}
		}
	}

	RenderableParticleBunchPtr createBunch(std::size_t cycleIndex)
	{
		return RenderableParticleBunchPtr(new RenderableParticleBunch(
					cycleIndex, getSeed(cycleIndex), _stage, _viewRotation, _direction));
	}

	int getSeed(std::size_t cycleIndex)
	{
		return _seeds[cycleIndex % _seeds.size()];
	}

	RenderableParticleBunchPtr getExistingBunchByIndex(std::size_t index)	
	{
		if (_bunches[0] != NULL && _bunches[0]->getIndex() == index)
		{
			return _bunches[0];
		}
		else if (_bunches[1] != NULL && _bunches[1]->getIndex() == index)
		{
			return _bunches[1];
		}
		
		return RenderableParticleBunchPtr();
	}

	void calculateBounds()
	{
		if (_bunches[0] != NULL)
		{
			// Get one of our seed values
			_bounds.includeAABB(_bunches[0]->getBounds());
		}

		if (_bunches[1] != NULL)
		{
			_bounds.includeAABB(_bunches[1]->getBounds());
		}
	}
};
typedef boost::shared_ptr<RenderableParticleStage> RenderableParticleStagePtr;

} // namespace

#endif /* _RENDERABLE_PARTICLE_STAGE_H_ */
