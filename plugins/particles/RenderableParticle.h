#ifndef _RENDERABLE_PARTICLE_H_
#define _RENDERABLE_PARTICLE_H_

#include "iparticles.h"
#include "irender.h"

#include "render.h"
#include "math/matrix.h"

#include <boost/random/linear_congruential.hpp>

namespace particles
{

// Seconds to milliseconds
#define SEC2MS(x) ((x)*1000)

class RenderableParticleBunch :
	public OpenGLRenderable
{
private:
	// The bunch index
	std::size_t _index;

	// The stage this bunch is part of
	const IParticleStage& _stage;

	struct VertexInfo
	{
		Vector3 vertex;			// The 3D coordinates of the point
		Vector2 texcoord;		// The UV coordinates
		Vector3 normal;			// The normals
		Colour4b colour;		// vertex colour

		VertexInfo()
		{}

		VertexInfo(const Vector3& vertex_, const Vector2& texcoord_) :
			vertex(vertex_),
			texcoord(texcoord_),
			normal(0,0,1),
			colour(1,1,1,1)
		{}
	};

	// The quads of this particle bunch
	typedef std::vector<VertexInfo> Vertices;
	Vertices _vertices;

public:
	// Each bunch has a defined zero-based index
	RenderableParticleBunch(std::size_t index, 
							std::size_t particleCount, 
							const IParticleStage& stage) :
		_vertices(particleCount*4), // 4 vertices per particle
		_stage(stage)
	{
		// Geometry is written in update(), just reserve the space
	}

	std::size_t getIndex() const
	{
		return _index;
	}

	// Update the particle geometry and render information. 
	// Time is specified in "local time", i.e. "time within the cycle", in msecs.
	void update(std::size_t time, boost::rand48& random)
	{
		// TODO
	}

	void render(const RenderInfo& info) const
	{
		if (_vertices.empty()) return;

		glVertexPointer(3, GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().vertex));
		glTexCoordPointer(2, GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().texcoord));
		glNormalPointer(GL_DOUBLE, sizeof(VertexInfo), &(_vertices.front().normal));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexInfo), &(_vertices.front().colour));

		// TODO: Use calculated start and end array indices, not all particles may be visible at the same time
		glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(_vertices.size()));
	}
};
typedef boost::shared_ptr<RenderableParticleBunch> RenderableParticleBunchPtr;

/**
 * greebo: Each particle stage generates its geometry in one or more cycles.
 * Each cycle comes as a bunch of quads with a defined lifespan. It's possible
 * for quads of one cycle to exist during the lifetime of the next cycle (if bunching 
 * is set to values below 1), but there can always be 2 bunches active at the same time.
 */
class RenerableParticleStage :
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

public:
	RenerableParticleStage(const IParticleStage& stage, boost::rand48& random) :
		_stage(stage),
		_numSeeds(32),
		_seeds(_numSeeds),
		_bunches(2) // two bunches 
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
	void update(std::size_t time)
	{
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

		// Consider the "origin" vector of this stage
		Vector3 origin = _stage.getOffset();

		// Generate N particles (disregard bunching for the moment)
		for (int p = 0; p < _stage.getCount(); ++p)
		{
			//pushQuad(origin, size);	
		}
	}
	
	// Generates a new quad using the given origin as centroid
	/*void pushQuad(const Vector3& origin, float size)
	{
		// Create a simple quad facing the z axis
		_vertices.push_back(VertexInfo(Vector3(origin.x() - size, origin.y() + size, origin.z()), Vector2(0,0)));
		_vertices.push_back(VertexInfo(Vector3(origin.x() + size, origin.y() + size, origin.z()), Vector2(1,0)));
		_vertices.push_back(VertexInfo(Vector3(origin.x() + size, origin.y() - size, origin.z()), Vector2(1,1)));
		_vertices.push_back(VertexInfo(Vector3(origin.x() - size, origin.y() - size, origin.z()), Vector2(0,1)));
	}*/
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
			RenerableParticleStagePtr renderableStage(new RenerableParticleStage(stage, _random));
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
