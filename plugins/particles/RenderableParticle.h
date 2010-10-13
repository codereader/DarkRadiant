#ifndef _RENDERABLE_PARTICLE_H_
#define _RENDERABLE_PARTICLE_H_

#include "iparticles.h"
#include "irender.h"

#include "render.h"
#include "math/matrix.h"
#include "math/pi.h"

#include <boost/random/linear_congruential.hpp>

namespace particles
{

// Seconds to milliseconds
#define SEC2MS(x) ((x)*1000)
#define MS2SEC(x) ((x)*0.001f)

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
		Vector4 colour;		// vertex colour

		VertexInfo()
		{}

		VertexInfo(const Vector3& vertex_, const Vector2& texcoord_) :
			vertex(vertex_),
			texcoord(texcoord_),
			normal(0,0,1),
			colour(1,1,1,1)
		{}

		VertexInfo(const Vector3& vertex_, const Vector2& texcoord_, const Vector4& colour_) :
			vertex(vertex_),
			texcoord(texcoord_),
			normal(0,0,1),
			colour(colour_)
		{}
	};

	struct Quad
	{
		VertexInfo verts[4];

		Quad()
		{}

		Quad(float size) 
		{
			verts[0] = VertexInfo(Vector3(-size, +size, 0), Vector2(0,0));
			verts[1] = VertexInfo(Vector3(+size, +size, 0), Vector2(1,0));
			verts[2] = VertexInfo(Vector3(+size, -size, 0), Vector2(1,1));
			verts[3] = VertexInfo(Vector3(-size, -size, 0), Vector2(0,1));
		}

		Quad(float size, float angle, const Vector4& colour = Vector4(1,1,1,1))
		{
			double cosPhi = cos(degrees_to_radians(angle));
			double sinPhi = sin(degrees_to_radians(angle));
			Matrix4 rotation = Matrix4::byColumns(
				cosPhi, -sinPhi, 0, 0,
				sinPhi, cosPhi, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1);

			verts[0] = VertexInfo(rotation.transform(Vector3(-size, +size, 0)).getProjected(), Vector2(0,0), colour);
			verts[1] = VertexInfo(rotation.transform(Vector3(+size, +size, 0)).getProjected(), Vector2(1,0), colour);
			verts[2] = VertexInfo(rotation.transform(Vector3(+size, -size, 0)).getProjected(), Vector2(1,1), colour);
			verts[3] = VertexInfo(rotation.transform(Vector3(-size, -size, 0)).getProjected(), Vector2(0,1), colour);
		}

		void translate(const Vector3& offset)
		{
			verts[0].vertex += offset;
			verts[1].vertex += offset;
			verts[2].vertex += offset;
			verts[3].vertex += offset;
		}
	};

	// The quads of this particle bunch
	typedef std::vector<Quad> Quads;
	Quads _quads;

	// The seed for our local randomiser, as passed by the parent stage
	int _randSeed;

	// The randomiser itself, which is reset everytime we rebuild the geometry
	boost::rand48 _random;

public:
	// Each bunch has a defined zero-based index
	RenderableParticleBunch(std::size_t index, 
							int randSeed,
							const IParticleStage& stage) :
		_index(index),
		_stage(stage),
		_quads(stage.getCount()), // 4 vertices per particle
		_randSeed(randSeed)
	{
		// Geometry is written in update(), just reserve the space
	}

	std::size_t getIndex() const
	{
		return _index;
	}

	// Update the particle geometry and render information. 
	// Time is specified in stage time without offset,in msecs.
	void update(std::size_t time)
	{
		_quads.clear();

		// Length of one cycle (duration + deadtime)
		std::size_t cycleMsec = static_cast<std::size_t>(_stage.getCycleMsec());

		if (cycleMsec == 0)
		{
			return;
		}

		// Normalise the global input time into local cycle time
		// The cycleTime may be larger than the _stage.cycleMsec argument if bunching is turned off
		std::size_t cycleTime = time - cycleMsec * _index;

		// Reset the random number generator using our stored seed
		_random.seed(_randSeed);

		// Consider the offset parameter
		const Vector3& offset = _stage.getOffset();

		Vector3 direction(0,1,0); // y direction

		// Calculate the time between each particle spawn
		// When bunching is set to 1 the spacing is 0, and vice versa.
		std::size_t stageDurationMsec = static_cast<std::size_t>(SEC2MS(_stage.getDuration()));

		float spawnSpacing = _stage.getBunching() * static_cast<float>(stageDurationMsec) / _stage.getCount();

		// This is the spacing between each particle
		std::size_t spawnSpacingMsec = static_cast<std::size_t>(spawnSpacing);

		// Load the flag whether to spawn particles at random locations
		bool distributeParticlesRandomly = _stage.getRandomDistribution();

		// Generate all particle quads, regardless of their visibility
		// Visibility is considered by not rendering particles that haven't been spawned yet
		for (std::size_t i = 0; i < static_cast<std::size_t>(_stage.getCount()); ++i)
		{
			// Consider bunching parameter
			std::size_t particleStartTimeMsec = i * spawnSpacingMsec;

			if (cycleTime < particleStartTimeMsec)
			{
				// This particle is not visible at the given time
				continue;
			}

			assert(particleStartTimeMsec < stageDurationMsec);  // some sanity checks

			// Get the "local particle time"
			std::size_t particleTime = cycleTime - particleStartTimeMsec;

			// Calculate the time fraction [0..1]
			float timeFraction = static_cast<float>(particleTime) / stageDurationMsec;

			// We need the particle time in seconds for the location/angle integrations
			float particleTimeSecs = MS2SEC(particleTime);

			// Calculate particle origin at time t, consider offset
			Vector3 particleOrigin = offset;

			// Consider particle distribution
			particleOrigin += getDistributionOffset(distributeParticlesRandomly);
			
			// Consider speed
			particleOrigin += direction * integrate(_stage.getSpeed(), particleTimeSecs);

			// Consider gravity, ignore "world" parameter for now
			particleOrigin += Vector3(0,0,-1) * _stage.getGravity() * particleTimeSecs * particleTimeSecs * 0.5f;

			// Get the initial angle value
			float angle = _stage.getInitialAngle();

			if (angle == 0)
			{
				// Use random angle
				angle = 360 * static_cast<float>(_random()) / boost::rand48::max_value;
			}

			// Past this point, no more "randomness" is required, so let's check if we still need
			// to render this particular particle. Don't dismiss particles too early, as each of them
			// will change the RNG state in the calculations above. These state changes are important for
			// all the subsequent particles.

			// Each particle has a lifetime of <stage duration> at maximum
			if (particleTime > stageDurationMsec)
			{
				continue; // particle has expired
			}

			// Calculate the time-dependent angle
			// according to docs, half the quads have negative rotation speed
			int rotFactor = i % 2 == 0 ? -1 : 1;
			angle += rotFactor * integrate(_stage.getRotationSpeed(), particleTimeSecs);

			// Calculate render colour
			Vector4 colour = _stage.getColour();

			// Consider fade index fraction, which can spawn particles already faded to some extent
			float fadeIndexFraction = _stage.getFadeIndexFraction();

			if (fadeIndexFraction > 0)
			{
				// greebo: The linear fading function goes like this:
				// frac(t) = (startFrac - t) / (startFrac - 1) with t in [0..1]
				// Boundary conditions: frac(1) = 1 and frac(startFrac) = 0

				// Use the particle index as "time", normalised to [0..1]
				// such that particle with higher index start more faded
				float pIdx = static_cast<float>(i) / _stage.getCount();

				// Calculate how much we should be faded already
				float startFrac = 1.0f - fadeIndexFraction;
				float frac = (startFrac - pIdx) / (startFrac - 1.0f);
				
				// Ignore negative fraction values, this also takes care that only
				// those particles with time >= fadeIndexFraction get faded.
				if (frac > 0)
				{
					colour = lerpColour(colour, _stage.getFadeColour(), frac);
				}
			}

			float fadeInFraction = _stage.getFadeInFraction();

			if (fadeInFraction > 0 && timeFraction <= fadeInFraction)
			{
				colour = lerpColour(_stage.getFadeColour(), _stage.getColour(), timeFraction / fadeInFraction); 
			}

			float fadeOutFraction = _stage.getFadeOutFraction();
			float fadeOutFractionInverse = 1.0f - fadeOutFraction;

			if (fadeOutFraction > 0 && timeFraction >= fadeOutFractionInverse)
			{
				colour = lerpColour(_stage.getColour(), _stage.getFadeColour(), (timeFraction - fadeOutFractionInverse) / fadeOutFraction);
			}

			pushQuad(particleOrigin, _stage.getSize().evaluate(timeFraction), angle, colour);
		}
	}

	void render(const RenderInfo& info) const
	{ 
		if (_quads.empty()) return;

		glVertexPointer(3, GL_DOUBLE, sizeof(VertexInfo), &(_quads.front().verts[0].vertex));
		glTexCoordPointer(2, GL_DOUBLE, sizeof(VertexInfo), &(_quads.front().verts[0].texcoord));
		glNormalPointer(GL_DOUBLE, sizeof(VertexInfo), &(_quads.front().verts[0].normal));
		glColorPointer(4, GL_DOUBLE, sizeof(VertexInfo), &(_quads.front().verts[0].colour));
		
		glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(_quads.size())*4);
	}

private:
	float integrate(const IParticleParameter& param, float time)
	{
		return (param.getTo() - param.getFrom()) / SEC2MS(_stage.getDuration()) * time*time * 0.5f + param.getFrom() * time;
	}

	Vector4 lerpColour(const Vector4& startColour, const Vector4& endColour, float fraction)
	{
		return startColour * (1.0f - fraction) + endColour * fraction;
	}

	Vector3 getDistributionOffset(bool distributeParticlesRandomly)
	{
		switch (_stage.getDistributionType())
		{
			// Rectangular distribution
			case IParticleStage::DISTRIBUTION_RECT:
			{
				// Factors to use for the random distribution
				float randX = 1.0f;
				float randY = 1.0f;
				float randZ = 1.0f;

				if (distributeParticlesRandomly)
				{
					// Rectangular spawn zone
					randX = 2 * static_cast<float>(_random()) / boost::rand48::max_value - 1.0f;
					randY = 2 * static_cast<float>(_random()) / boost::rand48::max_value - 1.0f;
					randZ = 2 * static_cast<float>(_random()) / boost::rand48::max_value - 1.0f;
				}

				// If random distribution is off, particles get spawned at <sizex, sizey, sizez>

				return Vector3(randX * _stage.getDistributionParm(0), 
							   randY * _stage.getDistributionParm(1), 
							   randZ * _stage.getDistributionParm(2));
			}

			case IParticleStage::DISTRIBUTION_CYLINDER:
			{
				// Get the cylinder dimensions
				float sizeX = _stage.getDistributionParm(0);
				float sizeY = _stage.getDistributionParm(1);
				float sizeZ = _stage.getDistributionParm(2);
				float ringFrac = _stage.getDistributionParm(3);

				// greebo: Some tests showed that for the cylinder type
				// the fourth parameter ("ringfraction") is only effective if >1,
				// it effectively scales the elliptic shape by that factor. 
				// Values < 1.0 didn't have any effect (?) Someone could double-check that.
				// Interestingly, the built-in particle editor doesn't really allow editing that parameter.
				if (ringFrac > 1.0f) 
				{
					sizeX *= ringFrac;
					sizeY *= ringFrac;
				}

				if (distributeParticlesRandomly)
				{
					// Get a random angle in [0..2pi]
					float angle = static_cast<float>(2*c_pi) * static_cast<float>(_random()) / boost::rand48::max_value;

					float xPos = cos(angle) * sizeX;
					float yPos = sin(angle) * sizeY;
					float zPos = sizeZ * (2 * static_cast<float>(_random()) / boost::rand48::max_value - 1.0f);

					return Vector3(xPos, yPos, zPos);
				}
				else
				{
					// Random distribution is off, particles get spawned at <sizex, sizey, sizez>
					return Vector3(sizeX, sizeY, sizeZ);
				}
			}

			// Default case, should not be reachable
			default:
				return Vector3(0,0,0);
		};
	}

	// Generates a new quad using the given origin as centroid, angle is in degrees
	void pushQuad(const Vector3& origin, float size, float angle, const Vector4& colour)
	{
		// Create a simple quad facing the z axis
		_quads.push_back(Quad(size, angle, colour));

		_quads.back().translate(origin);
	}
};
typedef boost::shared_ptr<RenderableParticleBunch> RenderableParticleBunchPtr;

/**
 * greebo: Each particle stage generates its geometry in one or more cycles.
 * Each cycle comes as a bunch of quads with a defined lifespan. It's possible
 * for quads of one cycle to exist during the lifetime of the next cycle (if bunching 
 * is set to values below 1), but there can always be 2 bunches active at the same time.
 */
class RenderableParticleStage :
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
	RenderableParticleStage(const IParticleStage& stage, boost::rand48& random) :
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

		// Make sure the correct bunches are allocated for this stage time
		ensureBunches(localtimeMsec);

		// The 0 bunch is the active one, the 1 bunch is the previous one if not null

		// Tell the particle batches to update their geometry
		if (_bunches[0] != NULL)
		{
			// Get one of our seed values
			_bunches[0]->update(localtimeMsec);
		}

		if (_bunches[1] != NULL)
		{
			_bunches[1]->update(localtimeMsec);
		}
	}

private:
	void ensureBunches(std::size_t localTimeMSec)
	{
		// Check which bunches is active at this time
		float cycleFrac = floor(static_cast<float>(localTimeMSec) / _stage.getCycleMsec());

		std::size_t curCycleIndex = static_cast<std::size_t>(cycleFrac);

		if (curCycleIndex == 0)
		{
			// This is the only active bunch (the first one), there is no previous cycle
			// it's possible that this one is already existing.
			if (_bunches[0] == NULL || _bunches[0]->getIndex() != curCycleIndex)
			{
				// First bunch is not matching, re-assign
				_bunches[0].reset(new RenderableParticleBunch(curCycleIndex, getSeed(curCycleIndex), _stage));
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

			std::size_t numCycles = static_cast<std::size_t>(_stage.getCycles());

			if (numCycles > 0 && curCycleIndex > numCycles)
			{
				// We've exceeded the maximum number of cycles
				_bunches[0].reset();
			}
			else if (cur != NULL)
			{
				_bunches[0] = cur;
			}
			else
			{
				_bunches[0].reset(new RenderableParticleBunch(curCycleIndex, getSeed(curCycleIndex), _stage));
			}

			if (numCycles > 0 && prevCycleIndex > numCycles)
			{
				// We've exceeded the maximum number of cycles
				_bunches[1].reset();
			}
			else if (prev != NULL)
			{
				_bunches[1] = prev;
			}
			else
			{
				_bunches[1].reset(new RenderableParticleBunch(prevCycleIndex, getSeed(prevCycleIndex), _stage));
			}
		}
	}

	int getSeed(std::size_t cycleIndex)
	{
		return _seeds[cycleIndex % _seeds.size()];
	}

	RenderableParticleBunchPtr getExistingBunchByIndex(std::size_t index)	
	{
		if (_bunches[0] != NULL && _bunches[0]->getIndex() == index)
		{
			return _bunches[0];
		}
		else if (_bunches[1] != NULL && _bunches[1]->getIndex() == index)
		{
			return _bunches[1];
		}
		
		return RenderableParticleBunchPtr();
	}
};
typedef boost::shared_ptr<RenderableParticleStage> RenderableParticleStagePtr;

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
