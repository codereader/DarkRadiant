#ifndef _RENDERABLE_PARTICLE_H_
#define _RENDERABLE_PARTICLE_H_

#include "iparticles.h"
#include "irender.h"

#include "render.h"
#include "math/matrix.h"

namespace particles
{

class RenerableParticleStage :
	public OpenGLRenderable
{
private:
	// The stage def we're rendering
	const IParticleStage& _stage;

	struct VertexInfo
	{
		Vector3 vertex;			// The 3D coordinates of the point
		Vector2 texcoord;		// The UV coordinates
		Vector3 normal;			// The normals

		VertexInfo()
		{}

		VertexInfo(const Vector3& vertex_, const Vector2& texcoord_) :
			vertex(vertex_),
			texcoord(texcoord_),
			normal(0,0,1)
		{}
	};

	std::vector<VertexInfo> _vertices;

public:
	RenerableParticleStage(const IParticleStage& stage) :
		_stage(stage)
	{}

	void render(const RenderInfo& info) const
	{
		glVertexPointer(3, GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().vertex));
		glTexCoordPointer(2, GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().texcoord));
		glNormalPointer(GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().normal));

		glDrawArrays(GL_QUADS, 0, _vertices.size());
	}

	// Generate particle geometry 
	void update(std::size_t time)
	{
		_vertices.clear();

		pushQuad(Vector3(0,0,0), 10);
	}
	
	// Generates a new quad using the given origin as centroid
	void pushQuad(const Vector3& origin, double size)
	{
		// Create a simple quad facing the z axis
		_vertices.push_back(VertexInfo(Vector3(origin.x() - size, origin.y() + size, origin.z()), Vector2(0,0)));
		_vertices.push_back(VertexInfo(Vector3(origin.x() + size, origin.y() + size, origin.z()), Vector2(1,0)));
		_vertices.push_back(VertexInfo(Vector3(origin.x() + size, origin.y() - size, origin.z()), Vector2(1,1)));
		_vertices.push_back(VertexInfo(Vector3(origin.x() - size, origin.y() - size, origin.z()), Vector2(0,1)));
	}
};
typedef boost::shared_ptr<RenerableParticleStage> RenerableParticleStagePtr;

class RenderableParticle :
	public IRenderableParticle
{
private:
	// The particle definition containing the stage info
	IParticleDefPtr _particleDef;

	typedef std::vector<RenerableParticleStagePtr> RenerableParticleStageList;

	// Particle stages using the same shader get grouped
	struct ParticleStageGroup
	{
		ShaderPtr shader;
		RenerableParticleStageList stages;
	};

	// Each captured shader can have one or more particle stages assigned to it
	typedef std::map<std::string, ParticleStageGroup> ShaderMap;
	ShaderMap _shaderMap;

public:
	RenderableParticle(const IParticleDefPtr& particleDef) :
		_particleDef(particleDef)
	{
		setupStages();
	}

	void update(std::size_t time, RenderSystem& renderSystem)
	{
		// TODO: Evaluate particle state

		// TODO: Update renderable geometry

		ensureShaders(renderSystem);

		// Traverse the stages and call update
		for (ShaderMap::const_iterator i = _shaderMap.begin(); i != _shaderMap.end(); ++i)
		{
			for (RenerableParticleStageList::const_iterator stage = i->second.stages.begin();
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

			for (RenerableParticleStageList::const_iterator stage = i->second.stages.begin();
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

	/**
	 * Get the particle definition used by this renderable.
	 */
	const IParticleDefPtr& getParticleDef() const
	{
		return _particleDef;
	}

	/**
	 * Set the particle definition.
	 */
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
			RenerableParticleStagePtr renderableStage(new RenerableParticleStage(stage));
			_shaderMap[materialName].stages.push_back(renderableStage);
		}
	}

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
