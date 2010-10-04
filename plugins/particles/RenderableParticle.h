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

	ShaderPtr _shader;

public:
	RenerableParticleStage(const IParticleStage& stage) :
		_stage(stage)
	{
		double len = 5;

		// Create a simple cube of unwelded vertices
		_vertices.push_back(VertexInfo(Vector3(-len, -len, -len), Vector2(0,0)));
		_vertices.push_back(VertexInfo(Vector3(+len, -len, -len), Vector2(1,0)));
		_vertices.push_back(VertexInfo(Vector3(+len, +len, -len), Vector2(1,1)));
		_vertices.push_back(VertexInfo(Vector3(-len, +len, -len), Vector2(0,1)));

		_vertices.push_back(VertexInfo(Vector3(-len, -len, -len), Vector2(0,0)));
		_vertices.push_back(VertexInfo(Vector3(-len, +len, -len), Vector2(1,0)));
		_vertices.push_back(VertexInfo(Vector3(-len, +len, +len), Vector2(1,1)));
		_vertices.push_back(VertexInfo(Vector3(-len, -len, +len), Vector2(0,1)));
		
		_vertices.push_back(VertexInfo(Vector3(+len, -len, -len), Vector2(0,0)));
		_vertices.push_back(VertexInfo(Vector3(+len, +len, -len), Vector2(1,0)));
		_vertices.push_back(VertexInfo(Vector3(+len, +len, +len), Vector2(1,1)));
		_vertices.push_back(VertexInfo(Vector3(+len, -len, +len), Vector2(0,1)));

		_vertices.push_back(VertexInfo(Vector3(-len, -len, +len), Vector2(0,0)));
		_vertices.push_back(VertexInfo(Vector3(+len, -len, +len), Vector2(1,0)));
		_vertices.push_back(VertexInfo(Vector3(+len, +len, +len), Vector2(1,1)));
		_vertices.push_back(VertexInfo(Vector3(-len, +len, +len), Vector2(0,1)));
	}

	const ShaderPtr& getShader() const
	{
		return _shader;
	}

	void setShader(const ShaderPtr& shader)
	{
		_shader = shader;
	}

	void render(const RenderInfo& info) const
	{
		glVertexPointer(3, GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().vertex));
		glTexCoordPointer(2, GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().texcoord));
		glNormalPointer(GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().normal));

		glDrawArrays(GL_QUADS, 0, _vertices.size());
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
