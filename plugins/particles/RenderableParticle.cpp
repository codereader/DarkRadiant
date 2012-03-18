#include "RenderableParticle.h"

#include <boost/foreach.hpp>

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
	setParticleDef(IParticleDefPtr());
}

// Time is in msecs
void RenderableParticle::update(const Matrix4& viewRotation)
{
	RenderSystemPtr renderSystem = _renderSystem.lock();

	if (!renderSystem) return; // no rendersystem there yet

	std::size_t time = renderSystem->getTime();

	// Invalidate our bounds information
	_bounds = AABB();

	// Make sure all shaders are constructed
	ensureShaders(*renderSystem);

	// greebo: Use the inverse matrix of the incoming matrix, this is enough to compensate
	// the camera rotation.
	Matrix4 invViewRotation = viewRotation.getInverse();

	// Traverse the stages and call update
	for (ShaderMap::const_iterator i = _shaderMap.begin(); i != _shaderMap.end(); ++i)
	{
		for (RenderableParticleStageList::const_iterator stage = i->second.stages.begin();
			 stage != i->second.stages.end(); ++stage)
		{
			(*stage)->update(time, invViewRotation);
		}
	}
}

// Front-end render methods
void RenderableParticle::renderSolid(RenderableCollector& collector,
                                     const VolumeTest& volume,
                                     const Matrix4& localToWorld,
                                     const IRenderEntity* entity) const
{
	for (ShaderMap::const_iterator i = _shaderMap.begin(); i != _shaderMap.end(); ++i)
	{
		assert(i->second.shader); // ensure we're realised

		collector.SetState(i->second.shader, RenderableCollector::eFullMaterials);

        // For each stage using this shader
        BOOST_FOREACH(RenderableParticleStagePtr stage, i->second.stages)
		{
			// Skip invisible stages
			if (!stage->getDef().isVisible()) continue;

			if (entity)
			{
				collector.addRenderable(*stage, localToWorld, *entity);
			}
			else
			{
				collector.addRenderable(*stage, localToWorld);
			}
		}
	}
}

void RenderableParticle::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	renderSolid(collector, volume, Matrix4::getIdentity(), NULL);
}

void RenderableParticle::renderWireframe(RenderableCollector& collector, const VolumeTest& volume, 
	const Matrix4& localToWorld, const IRenderEntity* entity) const
{
	// Does the same thing as renderSolid
	renderSolid(collector, volume, localToWorld, entity);
}

void RenderableParticle::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Does the same thing as renderSolid
	renderSolid(collector, volume);
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

	// The particle stages hold a const-reference to _direction
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
	for (ShaderMap::const_iterator i = _shaderMap.begin(); i != _shaderMap.end(); ++i)
	{
		for (RenderableParticleStageList::const_iterator stage = i->second.stages.begin();
			 stage != i->second.stages.end(); ++stage)
		{
			_bounds.includeAABB((*stage)->getBounds());
		}
	}
}

// Sort stages into groups sharing a material, without capturing the shader yet
void RenderableParticle::setupStages()
{
	_shaderMap.clear();

	if (_particleDef == NULL) return; // nothing to do.

	for (std::size_t i = 0; i < _particleDef->getNumStages(); ++i)
	{
		const IStageDef& stage = _particleDef->getStage(i);

		const std::string& materialName = stage.getMaterialName();

		if (_shaderMap.find(materialName) == _shaderMap.end())
		{
			_shaderMap.insert(ShaderMap::value_type(materialName, ParticleStageGroup()));
		}

		// Create a new renderable stage and add it to the shader
		RenderableParticleStagePtr renderableStage(new RenderableParticleStage(stage, _random, _direction, _entityColour));
		_shaderMap[materialName].stages.push_back(renderableStage);
	}
}

// Capture all shaders, if necessary
void RenderableParticle::ensureShaders(RenderSystem& renderSystem)
{
	for (ShaderMap::iterator i = _shaderMap.begin(); i != _shaderMap.end(); ++i)
	{
		if (i->second.shader == NULL)
		{
			i->second.shader = renderSystem.capture(i->first);
		}
	}
}

} // namespace
