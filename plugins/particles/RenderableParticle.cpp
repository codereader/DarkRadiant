#include "RenderableParticle.h"

namespace particles
{

RenderableParticle::RenderableParticle(const IParticleDefPtr& particleDef) :
	_particleDef(), // don't initialise the ptr yet
	_random(rand()), // use a random seed
	_direction(0,0,1) // default direction
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
void RenderableParticle::update(std::size_t time, RenderSystem& renderSystem, const Matrix4& viewRotation)
{
	// Invalidate our bounds information
	_bounds = AABB();

	// Make sure all shaders are constructed		
	ensureShaders(renderSystem);

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
void RenderableParticle::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const 
{
	for (ShaderMap::const_iterator i = _shaderMap.begin(); i != _shaderMap.end(); ++i)
	{
		assert(i->second.shader); // ensure we're realised

		collector.SetState(i->second.shader, RenderableCollector::eFullMaterials);

		for (RenderableParticleStageList::const_iterator stage = i->second.stages.begin();
			 stage != i->second.stages.end(); ++stage)
		{
			collector.addRenderable(**stage, Matrix4::getIdentity());
		}
	}
}

void RenderableParticle::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const 
{
	// Does the same thing as renderSolid
	renderSolid(collector, volume);
}

const IParticleDefPtr& RenderableParticle::getParticleDef() const
{
	return _particleDef;
}

void RenderableParticle::setParticleDef(const IParticleDefPtr& def) 
{
	if (_particleDef != NULL)
	{
		_particleDef->removeObserver(this);
	}

	_particleDef = def;

	if (_particleDef != NULL)
	{
		// Start monitoring this particle for reload events
		_particleDef->addObserver(this);
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

// Updates bounds from stages and returns the value
const AABB& RenderableParticle::getBounds() 
{
	if (!_bounds.isValid())
	{
		calculateBounds();
	}

	return _bounds;
}

// IParticleDef::Observer implementation
void RenderableParticle::onParticleReload()
{
	// Re-construct our renderable stages
	setupStages();
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
		const IParticleStage& stage = _particleDef->getParticleStage(i);

		const std::string& materialName = stage.getMaterialName();

		if (_shaderMap.find(materialName) == _shaderMap.end())
		{
			_shaderMap.insert(ShaderMap::value_type(materialName, ParticleStageGroup()));
		}

		// Create a new renderable stage and add it to the shader
		RenderableParticleStagePtr renderableStage(new RenderableParticleStage(stage, _random, _direction));
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
