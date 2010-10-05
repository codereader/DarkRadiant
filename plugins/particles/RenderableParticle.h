#ifndef _RENDERABLE_PARTICLE_H_
#define _RENDERABLE_PARTICLE_H_

#include "iparticles.h"
#include "irender.h"

#include "render.h"
#include "math/matrix.h"

namespace particles
{

// Seconds to milliseconds
#define SEC2MS(x) ((x)*1000)

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
		if (_vertices.empty()) return;

		glVertexPointer(3, GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().vertex));
		glTexCoordPointer(2, GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().texcoord));
		glNormalPointer(GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().normal));

		glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(_vertices.size()));
	}

	// Generate particle geometry, time is absolute in msecs 
	void update(std::size_t time)
	{
		_vertices.clear();

		// Check time offset (msecs)
		std::size_t timeOffset = static_cast<std::size_t>(SEC2MS(_stage.getTimeOffset()));

		if (time < timeOffset)
		{
			// We're still in the timeoffset zone where particle spawn is inhibited
			return;
		}

		// Time >= timeOffset at this point

		// Get rid of the time offset
		std::size_t localtimeMsec = time - timeOffset;

		// Transform time to local stage cycle time
		int stageCycleMsec = _stage.getCycleMsec();

		if (stageCycleMsec <= 0) 
		{
			return;
		}

		std::size_t cycleTimeMsec = localtimeMsec % stageCycleMsec;

		std::size_t durationMsec = static_cast<std::size_t>(SEC2MS(_stage.getDuration()));

		// Consider deadtime parameter
		if (cycleTimeMsec > durationMsec)
		{
			// Cycle time is past stage duration, don't render anything
			return;
		}

		// Calculate the time fraction [0..1]
		float timeFrac = static_cast<float>(cycleTimeMsec) / durationMsec;

		// Sanitise fraction if necessary
		if (timeFrac > 1.0f) timeFrac = 1.0f;
		if (timeFrac < 0.0f) timeFrac = 0.0f;

		// Get current quad size
		float size = _stage.getSize().evaluate(timeFrac);

		pushQuad(Vector3(0,0,0), size);
	}
	
	// Generates a new quad using the given origin as centroid
	void pushQuad(const Vector3& origin, float size)
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

	// Time is in msecs
	void update(std::size_t time, RenderSystem& renderSystem)
	{
		// Make sure all shaders are constructed		
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
			RenerableParticleStagePtr renderableStage(new RenerableParticleStage(stage));
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
