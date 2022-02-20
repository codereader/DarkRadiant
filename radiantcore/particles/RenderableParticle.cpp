#include "RenderableParticle.h"

namespace particles
{

RenderableParticle::RenderableParticle(const IParticleDefPtr& particleDef) :
	_particleDef(), // don't initialise the ptr yet
	_random(rand()), // use a random seed
	_direction(0,0,1), // default direction
	_entityColour(1,1,1) // default entity colour
{
	// Use this method, for observer handling
	setParticleDef(particleDef);
}

RenderableParticle::~RenderableParticle()
{
	// Clear the particle def reference (remove this class as observer)
    setParticleDef({});
}

// Time is in msecs
void RenderableParticle::update(const Matrix4& viewRotation, const Matrix4& localToWorld, IRenderEntity* entity)
{
	auto renderSystem = _renderSystem.lock();

	if (!renderSystem) return; // no rendersystem there yet

	auto time = renderSystem->getTime();

	// Invalidate our bounds information
	_bounds = AABB();

	// Make sure all shaders are constructed
	ensureShaders(*renderSystem);

	// greebo: Use the inverse matrix of the incoming matrix, this is enough to compensate
	// the camera rotation.
	auto invViewRotation = viewRotation.getInverse();

	// Traverse the stages and call update
	for (const auto& pair : _shaderMap)
	{
		for (const auto& stage : pair.second.stages)
		{
            if (!stage->getDef().isVisible())
            {
                stage->clear();
                continue;
            }

            // Update the particle geometry
            stage->update(time, invViewRotation);

            // Attach the geometry to the shader
            stage->submitGeometry(pair.second.shader, localToWorld);

            // Attach to the parent entity for lighting mode
            stage->attachToEntity(entity);
		}
	}
}

void RenderableParticle::clearRenderables()
{
    for (const auto& pair : _shaderMap)
    {
        for (const auto& stage : pair.second.stages)
        {
            stage->clear();
        }
    }
}

void RenderableParticle::onPreRender(const VolumeTest& volume)
{}


void RenderableParticle::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
}

void RenderableParticle::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	_renderSystem = renderSystem;
}

const IParticleDefPtr& RenderableParticle::getParticleDef() const
{
	return _particleDef;
}

void RenderableParticle::setParticleDef(const IParticleDefPtr& def)
{
	if (_particleDef)
	{
        _defConnection.disconnect();
	}

	_particleDef = def;

	if (_particleDef)
	{
		// Start monitoring this particle for reload events
		_defConnection = _particleDef->signal_changed().connect(
            sigc::mem_fun(this, &RenderableParticle::setupStages)
        );
	}

	// Re-construct our stage information
	setupStages();
}

void RenderableParticle::setMainDirection(const Vector3& direction)
{
	_direction = direction;

	// The particle stages hold a const-reference to _direction
	// so no further update is needed
}

void RenderableParticle::setEntityColour(const Vector3& colour)
{
	_entityColour = colour;

	// The particle stages hold a const-reference to _entityColour
	// so no further update is needed
}

// Updates bounds from stages and returns the value
const AABB& RenderableParticle::getBounds()
{
	if (!_bounds.isValid())
	{
		calculateBounds();
	}

	return _bounds;
}

void RenderableParticle::calculateBounds()
{
	for (const auto& pair : _shaderMap)
	{
		for (const auto& stage : pair.second.stages)
		{
			_bounds.includeAABB(stage->getBounds());
		}
	}
}

// Sort stages into groups sharing a material, without capturing the shader yet
void RenderableParticle::setupStages()
{
	_shaderMap.clear();

	if (!_particleDef) return; // nothing to do.

	for (std::size_t i = 0; i < _particleDef->getNumStages(); ++i)
	{
		const auto& stage = _particleDef->getStage(i);
		const auto& materialName = stage.getMaterialName();

		if (_shaderMap.find(materialName) == _shaderMap.end())
		{
			_shaderMap.emplace(materialName, ParticleStageGroup());
		}

		// Create a new renderable stage and add it to the shader
		auto renderableStage = std::make_shared<RenderableParticleStage>(stage, _random, _direction, _entityColour);
		_shaderMap[materialName].stages.emplace_back(std::move(renderableStage));
	}
}

// Capture all shaders, if necessary
void RenderableParticle::ensureShaders(RenderSystem& renderSystem)
{
	for (auto& pair : _shaderMap)
	{
		if (!pair.second.shader)
		{
			pair.second.shader = renderSystem.capture(pair.first);
		}
	}
}

} // namespace
