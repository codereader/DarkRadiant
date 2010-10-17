#ifndef _RENDERABLE_PARTICLE_BUNCH_H_
#define _RENDERABLE_PARTICLE_BUNCH_H_

#include "irender.h"
#include "iparticlestage.h"

#include "math/Vector2.h"
#include "math/Vector3.h"
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

		/**
		 * Create a new quad, using the given size and angle.
		 * Specify an optional vertex colour which is assigned to all four corners
		 * 
		 * [Optional]: s0 and sWidth are used for particle animation frames.
		 *
		 * @s0: defines the horizontal frame start coordinate in texture space (s).
		 * @sWidth: defines the width of this frame in texture space.
		 */
		Quad(float size, float angle, const Vector4& colour = Vector4(1,1,1,1), float s0 = 0.0f, float sWidth = 1.0f)
		{
			double cosPhi = cos(degrees_to_radians(angle));
			double sinPhi = sin(degrees_to_radians(angle));
			Matrix4 rotation = Matrix4::byColumns(
				cosPhi, -sinPhi, 0, 0,
				sinPhi, cosPhi, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1);

			verts[0] = VertexInfo(rotation.transform(Vector3(-size, +size, 0)).getProjected(), Vector2(s0,0), colour);
			verts[1] = VertexInfo(rotation.transform(Vector3(+size, +size, 0)).getProjected(), Vector2(s0 + sWidth,0), colour);
			verts[2] = VertexInfo(rotation.transform(Vector3(+size, -size, 0)).getProjected(), Vector2(s0 + sWidth,1), colour);
			verts[3] = VertexInfo(rotation.transform(Vector3(-size, -size, 0)).getProjected(), Vector2(s0,1), colour);
		}

		void translate(const Vector3& offset)
		{
			verts[0].vertex += offset;
			verts[1].vertex += offset;
			verts[2].vertex += offset;
			verts[3].vertex += offset;
		}

		void transform(const Matrix4& mat)
		{
			verts[0].vertex = mat.transform(verts[0].vertex).getVector3();
			verts[1].vertex = mat.transform(verts[1].vertex).getVector3();
			verts[2].vertex = mat.transform(verts[2].vertex).getVector3();
			verts[3].vertex = mat.transform(verts[3].vertex).getVector3();
		}
	};

	// The quads of this particle bunch
	typedef std::vector<Quad> Quads;
	Quads _quads;

	// The seed for our local randomiser, as passed by the parent stage
	int _randSeed;

	// The randomiser itself, which is reset everytime we rebuild the geometry
	boost::rand48 _random;

	// The flag whether to spawn particles at random locations (standard path calculation)
	bool _distributeParticlesRandomly;

	// The default direction of the emitter
	Vector3 _direction;

	// Stage-specific offset
	const Vector3& _offset;

	// The matrix to orient quads (owned by the RenderableParticleStage)
	const Matrix4& _viewRotation;

public:
	// Each bunch has a defined zero-based index
	RenderableParticleBunch(std::size_t index, 
							int randSeed,
							const IParticleStage& stage,
							const Matrix4& viewRotation) :
		_index(index),
		_stage(stage),
		_quads(),
		_randSeed(randSeed),
		_distributeParticlesRandomly(_stage.getRandomDistribution()),
		_direction(0,1,0),
		_offset(_stage.getOffset()),
		_viewRotation(viewRotation)
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

		// Reserve enough space for all the particles
		_quads.reserve(_stage.getCount()*4);

		// Normalise the global input time into local cycle time
		// The cycleTime may be larger than the _stage.cycleMsec argument if bunching is turned off
		std::size_t cycleTime = time - cycleMsec * _index;

		// Reset the random number generator using our stored seed
		_random.seed(_randSeed);

		// Calculate the time between each particle spawn
		// When bunching is set to 1 the spacing is 0, and vice versa.
		std::size_t stageDurationMsec = static_cast<std::size_t>(SEC2MS(_stage.getDuration()));

		float spawnSpacing = _stage.getBunching() * static_cast<float>(stageDurationMsec) / _stage.getCount();

		// This is the spacing between each particle
		std::size_t spawnSpacingMsec = static_cast<std::size_t>(spawnSpacing);

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

			// Get the "local particle time" in msecs
			std::size_t particleTime = cycleTime - particleStartTimeMsec;

			// Calculate the time fraction [0..1]
			float timeFraction = static_cast<float>(particleTime) / stageDurationMsec;

			// We need the particle time in seconds for the location/angle integrations
			float particleTimeSecs = MS2SEC(particleTime);

			// Calculate particle origin at time t
			Vector3 particleOrigin = getOrigin(particleTimeSecs);

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

			// Consider animation frames
			std::size_t animFrames = static_cast<std::size_t>(_stage.getAnimationFrames());

			if (animFrames > 0)
			{
				// At a given time, two particles can be visible at most
				float frameRate = _stage.getAnimationRate();

				// The time interval for cross-fading, fall back to entire duration * 3 for zero animation rates
				float frameIntervalSecs = frameRate > 0 ? 1.0f / frameRate : 3 * _stage.getDuration();

				// Calculate the current frame number, wrap around
				std::size_t curFrame = static_cast<std::size_t>(floor(particleTimeSecs / frameIntervalSecs)) % animFrames;

				// Wrap next frame around animationFrame count for looping
				std::size_t nextFrame = (curFrame + 1) % animFrames;

				// Calculate the time within the frame, relative to frame start
				float frameMicrotime = float_mod(particleTimeSecs, frameIntervalSecs);

				// As a fading lasts as long as the entire interval, the alpha gradient is the same as the FPS value
				// The "current" particle is always fading out, the nextFrame is fading in
				float curAlpha = 1.0f - frameRate * frameMicrotime;
				float nextAlpha = frameRate * frameMicrotime;

				Vector4 curColour = colour * curAlpha;
				Vector4 nextColour = colour* nextAlpha;

				// The width of a single frame in texture space
				float sWidth = 1.0f / animFrames;

				// Calculate the texture space for each frame and push the quads
				pushQuad(particleOrigin, _stage.getSize().evaluate(timeFraction), angle, curColour, sWidth * curFrame, sWidth);
				pushQuad(particleOrigin, _stage.getSize().evaluate(timeFraction), angle, nextColour, sWidth * nextFrame, sWidth);
			}
			else
			{
				// Generate a single quad using the given parameters
				pushQuad(particleOrigin, _stage.getSize().evaluate(timeFraction), angle, colour);
			}
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

	Vector3 getOrigin(float particleTimeSecs)
	{
		// Consider offset
		Vector3 particleOrigin = _offset;

		switch (_stage.getCustomPathType())
		{
		case IParticleStage::PATH_STANDARD: // Standard path calculation
			{
				// Consider particle distribution
				Vector3 distributionOffset = getDistributionOffset(_distributeParticlesRandomly);

				// Add this to the origin
				particleOrigin += distributionOffset;

				// Calculate particle direction, pass distribution offset (this is needed for DIRECTION_OUTWARD)
				Vector3 particleDirection = getDirection(_direction, distributionOffset);
				
				// Consider speed
				particleOrigin += particleDirection * integrate(_stage.getSpeed(), particleTimeSecs);
			}
			break;

		case IParticleStage::PATH_FLIES:
			{
				// greebo: "Flies" particles are moving on the surface of a sphere of radius <size>
				// The radial and axial speeds are chosen at random (but never 0) and are constant
				// during the lifetime of a particle. Starting position appears to be random,
				// but different to the "distribution sphere" type (i.e. it is not evenly distributed,
				// instead the particles seem to bunch themselves at the poles).

				// Sphere radius
				float radius = _stage.getCustomPathParm(2);

				// Generate starting conditions speed (+/-50%)
				float rand = 2 * (static_cast<float>(_random()) / boost::rand48::max_value) - 1.0f;
				float radialSpeedFactor = 1.0f + 0.5f * rand * rand;

				// greebo: factor 0.4 is empirical, I measured a few D3 particles for their circulation times
				float radialSpeed = _stage.getCustomPathParm(0) * 0.4f;

				rand = 2 * (static_cast<float>(_random()) / boost::rand48::max_value) - 1.0f;
				float axialSpeedFactor = 1.0f + 0.5f * rand * rand;
				float axialSpeed = _stage.getCustomPathParm(1) * 0.4f;

				float phi0 = 2 * static_cast<float>(c_pi) * static_cast<float>(_random()) / boost::rand48::max_value;
				float theta0 = static_cast<float>(c_pi) * static_cast<float>(_random()) / boost::rand48::max_value;

				// Calculate angles at the given particleTime
				float phi = phi0 + axialSpeed * particleTimeSecs;
				float theta = theta0 + radialSpeed * particleTimeSecs;

				// Move the particle origin
				particleOrigin += Vector3(radius * cos(theta) * sin(phi), radius * sin(theta) * sin(phi), radius * cos(phi));
			}
			break;

		case IParticleStage::PATH_HELIX:
			{
				// greebo: Helical movement is describing an elliptic cylinder, its shape is determined by
				// sizeX, sizeY and sizeZ. Particles are spawned randomly on that cylinder surface,
				// their velocities (radial and axial) are also random (both negative and positive
				// velocities are allowed).

				float sizeX = _stage.getCustomPathParm(0);
				float sizeY = _stage.getCustomPathParm(1);
				float sizeZ = _stage.getCustomPathParm(2);

				float radialSpeed = _stage.getCustomPathParm(3) * (2 * (static_cast<float>(_random()) / boost::rand48::max_value) - 1.0f);
				float axialSpeed = _stage.getCustomPathParm(4) * (2 * (static_cast<float>(_random()) / boost::rand48::max_value) - 1.0f);

				float phi0 = 2 * static_cast<float>(c_pi) * static_cast<float>(_random()) / boost::rand48::max_value;
				float z0 = sizeZ * (2 * (static_cast<float>(_random()) / boost::rand48::max_value) - 1.0f);

				float x = sizeX * cos(phi0 + radialSpeed * particleTimeSecs);
				float y = sizeY * sin(phi0 + radialSpeed * particleTimeSecs);
				float z = z0 + axialSpeed * particleTimeSecs;

				particleOrigin += Vector3(x, y, z);
			}
			break;

		case IParticleStage::PATH_ORBIT:
		case IParticleStage::PATH_DRIP:
			// These are actually unsupported by the engine ("bad path type")
			globalWarningStream() << "Unsupported path type (drip/orbit)." << std::endl;
			break;

		default:
			// Nothing
			break;
		};

		return particleOrigin;
	}

	// baseDirection should be normalised and not degenerate
	Vector3 getDirection(const Vector3& baseDirection, const Vector3& distributionOffset)
	{
		if (baseDirection.getLengthSquared() == 0)
		{
			return Vector3(0,0,1); // degenerate input
		}

		switch (_stage.getDirectionType())
		{
		case IParticleStage::DIRECTION_CONE:
			{
				Vector3 dir = baseDirection.getNormalised();

				// Find a normal vector to the base direction
				Vector3 base1 = abs(dir.x()) < abs(dir.y()) ? 
					(abs(dir.x()) < abs(dir.z()) ? Vector3(1,0,0) : Vector3(0,0,1)) : 
					(abs(dir.y()) < abs(dir.z()) ? Vector3(0,1,0) : Vector3(0,0,1));

				base1 = dir.crossProduct(base1);
				base1.normalise();

				// Another vector perpendicular to baseDir, forming the second base
				Vector3 base2 = dir.crossProduct(base1);

				// Pick a random point in the disc of radius discRadius
				float coneAngle = _stage.getDirectionParm(0);
				float discRadius = tan(coneAngle);

				// Use sqrt(r) to fix bunching at the disc center
				float r = discRadius * sqrt(static_cast<float>(_random()) / boost::rand48::max_value);
				float phi = 2 * static_cast<float>(c_pi) * static_cast<float>(_random()) / boost::rand48::max_value;

				Vector3 endPoint = baseDirection + base1 * r * cos(phi) + base2 * r * sin(phi);
				
				return endPoint.getNormalised();
			}
		case IParticleStage::DIRECTION_OUTWARD:
			{
				// This heavily relies on particles being distributed randomly within the spawn area
				Vector3 direction = distributionOffset.getNormalised();

				// Consider upwards bias
				direction.z() += _stage.getDirectionParm(0);

				return direction;
			}
		default:
			return Vector3(0,0,1);
		};
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

			case IParticleStage::DISTRIBUTION_SPHERE:
			{
				// Get the sphere dimensions
				float maxX = _stage.getDistributionParm(0);
				float maxY = _stage.getDistributionParm(1);
				float maxZ = _stage.getDistributionParm(2);
				float ringFrac = _stage.getDistributionParm(3);

				float minX = maxX * ringFrac;
				float minY = maxY * ringFrac;
				float minZ = maxZ * ringFrac;

				if (distributeParticlesRandomly)
				{
					// The following is modeled after http://mathworld.wolfram.com/SpherePointPicking.html
					float u = static_cast<float>(_random()) / boost::rand48::max_value;
					float v = static_cast<float>(_random()) / boost::rand48::max_value;

					float theta = 2 * static_cast<float>(c_pi) * u;
					float phi = acos(2*v - 1);

					// Take the sqrt(radius) to correct bunching at the center of the sphere
					float r = sqrt(static_cast<float>(_random()) / boost::rand48::max_value);

					float x = (minX + (maxX - minX) * r) * cos(theta) * sin(phi);
					float y = (minY + (maxY - minY) * r) * sin(theta) * sin(phi);
					float z = (minZ + (maxZ - minZ) * r) * cos(phi);

					return Vector3(x,y,z);
				}
				else
				{
					// Random distribution is off, particles get spawned at <sizex, sizey, sizez>
					return Vector3(maxX, maxY, maxZ);
				}
			}

			// Default case, should not be reachable
			default:
				return Vector3(0,0,0);
		};
	}

	// Generates a new quad using the given origin as centroid, angle is in degrees
	void pushQuad(const Vector3& origin, float size, float angle, const Vector4& colour, float s0 = 0.0f, float sWidth = 1.0f)
	{
		// Create a simple quad facing the z axis
		_quads.push_back(Quad(size, angle, colour, s0, sWidth));
		_quads.back().transform(_viewRotation);
		_quads.back().translate(origin);
	}
};
typedef boost::shared_ptr<RenderableParticleBunch> RenderableParticleBunchPtr;

} // namespace

#endif /* _RENDERABLE_PARTICLE_BUNCH_H_ */
