#ifndef _RENDERABLE_PARTICLE_H_
#define _RENDERABLE_PARTICLE_H_

#include "RenderableParticleStage.h"

#include "iparticles.h"
#include "irender.h"

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

public:
	RenderableParticle(const IParticleDefPtr& particleDef) :
		_particleDef(particleDef),
		_random(rand()) // use a random seed
	{
		setupStages();
	}

	// Time is in msecs
	void update(std::size_t time, RenderSystem& renderSystem)
	{
		// Make sure all shaders are constructed		
		ensureShaders(renderSystem);

		// Traverse the stages and call update
		for (ShaderMap::const_iterator i = _shaderMap.begin(); i != _shaderMap.end(); ++i)
		{
			for (RenderableParticleStageList::const_iterator stage = i->second.stages.begin();
				 stage != i->second.stages.end(); ++stage)
			{
				(*stage)->update(time);
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

private:
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
			RenderableParticleStagePtr renderableStage(new RenderableParticleStage(stage, _random));
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
