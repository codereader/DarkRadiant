#include "RenderableParticleStage.h"

namespace particles
{

RenderableParticleStage::RenderableParticleStage(
		const IStageDef& stage,
		Rand48& random,
		const Vector3& direction,
		const Vector3& entityColour) :
	_stageDef(stage),
	_numSeeds(32),
	_seeds(_numSeeds),
	_bunches(2), // two bunches
	_viewRotation(Matrix4::getIdentity()), // is re-calculated each update anyway
    _localToWorld(Matrix4::getIdentity()),
	_direction(direction),
	_entityColour(entityColour)
{
	// Generate our vector of random numbers used seed particle bunches
	// using the random number generator as provided by our parent particle system
	for (std::size_t i = 0; i < _numSeeds; ++i)
	{
		_seeds[i] = random();
	}
}

// Generate particle geometry, time is absolute in msecs
void RenderableParticleStage::update(std::size_t time, const Matrix4& viewRotation)
{
	// Invalidate our bounds information
	_bounds = AABB();

	// Check time offset (msecs)
	std::size_t timeOffset = static_cast<std::size_t>(SEC2MS(_stageDef.getTimeOffset()));

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
	if (_bunches[0])
	{
		// Get one of our seed values
		_bunches[0]->update(localtimeMsec);
	}

	if (_bunches[1])
	{
		_bunches[1]->update(localtimeMsec);
	}
}

void RenderableParticleStage::submitGeometry(const ShaderPtr& shader, const Matrix4& localToWorld)
{
    _localToWorld = localToWorld;

    RenderableGeometry::update(shader);
}

std::size_t RenderableParticleStage::getNumQuads() const
{
    return (_bunches[0] ? _bunches[0]->getNumQuads() : 0) +
           (_bunches[1] ? _bunches[1]->getNumQuads() : 0);
}

void RenderableParticleStage::updateGeometry()
{
    std::vector<render::RenderVertex> vertices;
    std::vector<unsigned int> indices;

    auto numQuads = getNumQuads();

    if (numQuads == 0)
    {
        updateGeometryWithData(render::GeometryType::Triangles, vertices,
                                                   indices);
        return;
    }

    vertices.reserve(numQuads * 4);
    indices.reserve(numQuads * 6);

    if (_bunches[0])
    {
        _bunches[0]->addVertexData(vertices, indices, _localToWorld);
    }

    if (_bunches[1])
    {
        _bunches[1]->addVertexData(vertices, indices, _localToWorld);
    }

    updateGeometryWithData(render::GeometryType::Triangles, vertices, indices);
}

const AABB& RenderableParticleStage::getBounds()
{
	if (!_bounds.isValid())
	{
		calculateBounds();
	}

	return _bounds;
}

const IStageDef& RenderableParticleStage::getDef() const
{
	return _stageDef;
}

void RenderableParticleStage::calculateStageViewRotation(const Matrix4& viewRotation)
{
	switch (_stageDef.getOrientationType())
	{
	case IStageDef::ORIENTATION_AIMED:
		_viewRotation = viewRotation;
		break;

	case IStageDef::ORIENTATION_VIEW:
		_viewRotation = viewRotation;
		break;

	case IStageDef::ORIENTATION_X:
		// Rotate the z vector such that it faces the x axis, and use that as transform
		// To keep the up/down orientation of the material, rotate it 90 degrees around z
		// before applying the z-to-x tilt (issue #4792)
        _viewRotation = Matrix4::getRotation(Vector3(0, 0, 1), Vector3(1, 0, 0))
                                .getMultipliedBy(
                                    Matrix4::getRotationAboutZ(math::Degrees(-90))
                                );
        break;

	case IStageDef::ORIENTATION_Y:
		// Rotate the z vector such that it faces the y axis, and use that as transform
		_viewRotation = Matrix4::getRotation(Vector3(0,0,1), Vector3(0,1,0));
		break;

	case IStageDef::ORIENTATION_Z:
		// Particles are already facing the z axis by default
		_viewRotation = Matrix4::getIdentity();
		break;

	default:
		_viewRotation = Matrix4::getIdentity();
	};
}

void RenderableParticleStage::ensureBunches(std::size_t localTimeMSec)
{
	// Check which bunches is active at this time
	float cycleFrac = floor(static_cast<float>(localTimeMSec) / _stageDef.getCycleMsec());

	std::size_t curCycleIndex = static_cast<std::size_t>(cycleFrac);

	if (curCycleIndex == 0)
	{
		// This is the only active bunch (the first one), there is no previous cycle
		// it's possible that this one is already existing.
		if (!_bunches[0] || _bunches[0]->getIndex() != curCycleIndex)
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

		std::size_t numCycles = static_cast<std::size_t>(_stageDef.getCycles());

		if (numCycles > 0 && curCycleIndex > numCycles)
		{
			// We've exceeded the maximum number of cycles
			_bunches[0].reset();
		}
		else if (cur)
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
		else if (prev)
		{
			_bunches[1] = prev;
		}
		else
		{
			_bunches[1] = createBunch(prevCycleIndex);
		}
	}
}

RenderableParticleBunchPtr RenderableParticleStage::createBunch(std::size_t cycleIndex)
{
	return std::make_shared<RenderableParticleBunch>(cycleIndex, getSeed(cycleIndex), 
        _stageDef, _viewRotation, _direction, _entityColour);
}

Rand48::result_type RenderableParticleStage::getSeed(std::size_t cycleIndex)
{
	return _seeds[cycleIndex % _seeds.size()];
}

RenderableParticleBunchPtr RenderableParticleStage::getExistingBunchByIndex(std::size_t index)
{
	if (_bunches[0] && _bunches[0]->getIndex() == index)
	{
		return _bunches[0];
	}

    if (_bunches[1] && _bunches[1]->getIndex() == index)
	{
		return _bunches[1];
	}

	return RenderableParticleBunchPtr();
}

void RenderableParticleStage::calculateBounds()
{
	if (_bunches[0])
	{
		// Get one of our seed values
		_bounds.includeAABB(_bunches[0]->getBounds());
	}

	if (_bunches[1])
	{
		_bounds.includeAABB(_bunches[1]->getBounds());
	}
}

} // namespace
