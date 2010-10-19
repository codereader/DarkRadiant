#ifndef _RENDERABLE_PARTICLE_H_
#define _RENDERABLE_PARTICLE_H_

#include "RenderableParticleStage.h"

#include "iparticles.h"
#include "irender.h"

#include "math/aabb.h"
#include "render.h"
#include <boost/random/linear_congruential.hpp>

namespace particles
{

class RenderableParticle :
	public IRenderableParticle
{
private:
	// The particle definition containing the stage info
	IParticleDefPtr _particleDef;

	typedef std::vector<RenderableParticleStagePtr> RenderableParticleStageList;

	// Particle stages using the same shader get grouped
	struct ParticleStageGroup
	{
		ShaderPtr shader;
		RenderableParticleStageList stages;
	};

	// Each captured shader can have one or more particle stages assigned to it
	typedef std::map<std::string, ParticleStageGroup> ShaderMap;
	ShaderMap _shaderMap;

	// The random number generator, this is used to generate "constant"
	// starting values for each bunch of particles. This enables us
	// to go back in time when rendering the particle stage.
	boost::rand48 _random;

	// The particle direction, usually set by the emitter entity or the preview
	Vector3 _direction;

	// Holds the bounds of all stages at the current time. Will be updated
	// by calls to getBounds(), otherwise might hold outdated bounds information.
	AABB _bounds;

public:
	RenderableParticle(const IParticleDefPtr& particleDef) :
		_particleDef(particleDef),
		_random(rand()) // use a random seed
	{
		setupStages();
	}

	// Time is in msecs
	void update(std::size_t time, RenderSystem& renderSystem, const Matrix4& viewRotation)
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
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const 
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

	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const 
	{
		// Does the same thing as renderSolid
		renderSolid(collector, volume);
	}

	const IParticleDefPtr& getParticleDef() const
	{
		return _particleDef;
	}

	void setParticleDef(const IParticleDefPtr& def) 
	{
		_particleDef = def;
		setupStages();
	}

	void setMainDirection(const Vector3& direction)
	{
		_direction = direction;

		// The particle stages hold a const-reference to _direction
		// so no further update is needed
	}

	// Updates bounds from stages and returns the value
	const AABB& getBounds() 
	{
		if (!_bounds.isValid())
		{
			calculateBounds();
		}

		return _bounds;
	}

private:
	void calculateBounds()
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
	void setupStages()
	{
		_shaderMap.clear();

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
	void ensureShaders(RenderSystem& renderSystem)
	{
		for (ShaderMap::iterator i = _shaderMap.begin(); i != _shaderMap.end(); ++i)
		{
			if (i->second.shader == NULL)
			{
				i->second.shader = renderSystem.capture(i->first);
			}
		}
	}
};
typedef boost::shared_ptr<RenderableParticle> RenderableParticlePtr;

} // namespace

#endif /* _RENDERABLE_PARTICLE_H_ */
