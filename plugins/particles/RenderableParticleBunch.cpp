#include "RenderableParticleBunch.h"

#include "itextstream.h"
#include "math/pi.h"

#include "string/string.h"

namespace particles
{

void RenderableParticleBunch::update(std::size_t time)
{
	_bounds = AABB();
	_quads.clear();
	_debugInfo.clear();

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

		// // Generate four random numbers for custom path calcs, this is needed in getOriginAndVelocity
		float rands[4] = { static_cast<float>(_random()) / boost::rand48::max_value, 
						   static_cast<float>(_random()) / boost::rand48::max_value, 
						   static_cast<float>(_random()) / boost::rand48::max_value, 
						   static_cast<float>(_random()) / boost::rand48::max_value };

		// Calculate particle origin at time t
		Vector3 particleOrigin;
		Vector3 particleVelocity;
		getOriginAndVelocity(particleTimeSecs, timeFraction, rands, particleOrigin, particleVelocity);

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

		// Calculate render colour for this particle (pass the index)
		Vector4 colour = getColour(timeFraction, i);

		// Consider quad size
		float size = _stage.getSize().evaluate(timeFraction);

		// Consider aspect ratio
		float aspect = _stage.getAspect().evaluate(timeFraction);

		// Consider animation frames
		std::size_t animFrames = static_cast<std::size_t>(_stage.getAnimationFrames());

		// For aimed orientation, we need to override particle height and aspect
		if (_stage.getOrientationType() == IParticleStage::ORIENTATION_AIMED)
		{
			float aimedTime = _stage.getOrientationParm(0);	// time
			int trails = static_cast<int>(_stage.getOrientationParm(1)); // trails

			if (trails < 0) 
			{
				trails = 0;
			}

			// The time parameter defaults to 0.5 if not specified
			if (aimedTime == 0.0f)
			{
				aimedTime = 0.5f;
			}

			// The time delta to step into the past
			int numQuads = trails + 1;

			// The time delta between quads
			float timeStep = aimedTime / numQuads;

			Vector3 lastOrigin = particleOrigin;
			
			for (int i = 1; i <= numQuads; ++i)
			{
				// Get the time of the i-th particle in seconds, plus the fraction
				float timeSecs = particleTimeSecs - timeStep * i;
				float timeFrac = SEC2MS(timeSecs) / stageDurationMsec;

				// Get origin and velocity at that time
				Vector3 origin;
				Vector3 velocity;
				getOriginAndVelocity(timeSecs, timeFrac, rands, origin, velocity);

				float height = static_cast<float>((origin - lastOrigin).getLength());

				aspect = 2 * size / height;

				float particleSize = height * 0.5f;

				// Calculate the vertical texture coordinates
				float tWidth = 1.0f / static_cast<float>(numQuads);
				float t0 = (i - 1) * tWidth;

				// The matrix is special for each particle. For helix and other path types
				// it's necessary to apply the same matrix to each vertex sharing the same 3D location.

				// Calculate the matrix for the "older" two vertices of the quad
				Matrix4 local2aimed = getAimedMatrix(velocity);

				{
					const Vector3& normal = local2aimed.z().getVector3();

					// Ignore the angle for aimed orientation
					_quads.push_back(ParticleQuad(particleSize, aspect, 0, colour, normal, 0, 1, t0, tWidth));

					ParticleQuad& curQuad = _quads.back();

					// Apply a slight origin correction before rotating them, particles are not centered around 0,0,0 here
					curQuad.translate(Vector3(0, -height*0.5f, 0));
					curQuad.transform(local2aimed);				
					curQuad.translate(origin);

					// Glue the first row of vertices to the last quad, if applicable
					if (i > 1)
					{
						ParticleQuad& prevQuad = _quads[_quads.size() - 2];

						// Take the midpoint 
						curQuad.verts[0].vertex = (curQuad.verts[0].vertex + prevQuad.verts[3].vertex) * 0.5f;
						curQuad.verts[1].vertex = (curQuad.verts[1].vertex + prevQuad.verts[2].vertex) * 0.5f;

						// Snap the "previous" vertices to the same spot
						prevQuad.verts[3].vertex = curQuad.verts[0].vertex;
						prevQuad.verts[2].vertex = curQuad.verts[1].vertex;
					}
				}

				lastOrigin = origin;
			}
		}
		else if (animFrames > 0)
		{
			pushAnimatedQuads(animFrames, particleOrigin, particleVelocity, size, aspect, angle, colour, particleTimeSecs);
		}
		else
		{
			// Generate a single quad using the given parameters
			pushQuad(particleOrigin, particleVelocity, size, aspect, angle, colour);
		}
	}
}

void RenderableParticleBunch::render(const RenderInfo& info) const
{ 
	if (_quads.empty()) return;

	glVertexPointer(3, GL_DOUBLE, sizeof(ParticleQuad::Vertex), &(_quads.front().verts[0].vertex));
	glTexCoordPointer(2, GL_DOUBLE, sizeof(ParticleQuad::Vertex), &(_quads.front().verts[0].texcoord));
	glNormalPointer(GL_DOUBLE, sizeof(ParticleQuad::Vertex), &(_quads.front().verts[0].normal));
	glColorPointer(4, GL_DOUBLE, sizeof(ParticleQuad::Vertex), &(_quads.front().verts[0].colour));
	
	glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(_quads.size())*4);
}

const AABB& RenderableParticleBunch::getBounds()
{
	if (!_bounds.isValid())
	{
		calculateBounds();
	}

	return _bounds;
}

Matrix4 RenderableParticleBunch::getAimedMatrix(const Vector3& particleVelocity)
{
	// Get the velocity direction in object space, use the same velocity for all trailing quads
	Vector3 vel = particleVelocity.getNormalised();

	// Construct the matrices
	const Matrix4& camera2Object = _viewRotation;

	// The matrix rotating the particle into velocity space
	Matrix4 object2Vel = Matrix4::getRotation(Vector3(0,1,0), vel);

	// Transform the view (-z) vector into object space
	Vector3 view = camera2Object.transform(Vector3(0,0,-1)).getVector3();

	// Project the view vector onto the plane defined by the velocity vector
	Vector3 viewProj = view - vel * view.dot(vel);
	
	// This is the particle normal in object space (after being oriented such that y || velocity)
	Vector3 z = object2Vel.z().getVector3();

	// The particle needs to be rotated by this angle around the velocity axis
	double aimedAngle = z.angle(-viewProj);

	// Use the cross to check whether to rotate in negative or positive direction
	if (z.crossProduct(-viewProj).dot(vel) > 0)
	{
		aimedAngle *= -1;
	}

	// Calculate the rotation of the particle normal towards the view vector, around the velocity axis
	Matrix4 vel2aimed = Matrix4::getRotation(vel, aimedAngle);

	// Combine the matrices object2Vel => vel2aimed;
	return vel2aimed.getMultipliedBy(object2Vel);
}

Vector4 RenderableParticleBunch::getColour(float timeFraction, std::size_t particleIndex)
{
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
		float pIdx = static_cast<float>(particleIndex) / _stage.getCount();

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

	return colour;
}

void RenderableParticleBunch::getOriginAndVelocity(float particleTimeSecs, float timeFraction, float rands[4],
												   Vector3& particleOrigin, Vector3& particleVelocity)
{
	// Consider offset as starting point
	particleOrigin = _offset;

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

			// Save velocity for later use
			particleVelocity = particleDirection * _stage.getSpeed().evaluate(timeFraction);
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
			float rand = 2 * rands[0] - 1.0f;
			float radialSpeedFactor = 1.0f + 0.5f * rand * rand;

			// greebo: factor 0.4 is empirical, I measured a few D3 particles for their circulation times
			float radialSpeed = _stage.getCustomPathParm(0) * radialSpeedFactor * 0.4f;

			rand = 2 * rands[1] - 1.0f;
			float axialSpeedFactor = 1.0f + 0.5f * rand * rand;
			float axialSpeed = _stage.getCustomPathParm(1) * axialSpeedFactor * 0.4f;

			float phi0 = 2 * static_cast<float>(c_pi) * rands[2];
			float theta0 = static_cast<float>(c_pi) * rands[3];

			// Calculate angles at the given particleTime
			float phi = phi0 + axialSpeed * particleTimeSecs;
			float theta = theta0 + radialSpeed * particleTimeSecs;

			// Pre-calculate the sin/cos values
			float cosPhi = cos(phi);
			float sinPhi = sin(phi);
			float cosTheta = cos(theta);
			float sinTheta = sin(theta);

			// Move the particle origin
			particleOrigin += Vector3(radius * cosTheta * sinPhi, radius * sinTheta * sinPhi, radius * cosPhi);

			// Calculate the time derivative as velocity
			particleVelocity = Vector3(
				radius * (cosTheta * cosPhi * axialSpeed - sinTheta * sinPhi * radialSpeed),	// dx/dt
				radius * (cosTheta * sinPhi * radialSpeed + sinTheta * cosPhi * axialSpeed),	// dy/dt
				radius * (-1) * sinTheta * axialSpeed											// dz/dt
			);
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

			float radialSpeed = _stage.getCustomPathParm(3) * (2 * rands[0] - 1.0f);
			float axialSpeed = _stage.getCustomPathParm(4) * (2 * rands[1] - 1.0f);

			float phi0 = 2 * static_cast<float>(c_pi) * rands[2];
			float z0 = sizeZ * (2 * rands[3] - 1.0f);

			float sinPhi = sin(phi0 + radialSpeed * particleTimeSecs);
			float cosPhi = cos(phi0 + radialSpeed * particleTimeSecs);

			float x = sizeX * cosPhi;
			float y = sizeY * sinPhi;
			float z = z0 + axialSpeed * particleTimeSecs;

			particleOrigin += Vector3(x, y, z);

			particleVelocity = Vector3(
				sizeX * (-1) * sinPhi * radialSpeed,	// dx/dt
				sizeY * cosPhi * radialSpeed,			// dy/dt
				axialSpeed								// dz/dt
			);
		}
		break;

	case IParticleStage::PATH_ORBIT:
	case IParticleStage::PATH_DRIP:
		// These are actually unsupported by the engine ("bad path type")
		globalWarningStream() << "Unsupported path type (drip/orbit)." << std::endl;
		particleVelocity.set(0,0,0);
		break;

	default:
		// Nothing
		particleVelocity.set(0,0,0);
		break;
	};

	// Consider gravity
	// if "world" is set, use -z as gravity direction, otherwise use the reverse emitter direction
	Vector3 gravity = _stage.getWorldGravityFlag() ? Vector3(0,0,-1) : -_direction.getNormalised();

	particleOrigin += gravity * _stage.getGravity() * particleTimeSecs * particleTimeSecs * 0.5f;

	// Add gravity to particle speed result
	particleVelocity += gravity * _stage.getGravity() * particleTimeSecs;
}

// baseDirection should be normalised and not degenerate
Vector3 RenderableParticleBunch::getDirection(const Vector3& baseDirection, const Vector3& distributionOffset)
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

			return direction; // CHECKME: Use .getNormalised() ?
		}
	default:
		return Vector3(0,0,1);
	};
}

Vector3 RenderableParticleBunch::getDistributionOffset(bool distributeParticlesRandomly)
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
void RenderableParticleBunch::pushQuad(const Vector3& origin, const Vector3& velocity, 
									   float size, float aspect, float angle, 
									   const Vector4& colour, float s0, float sWidth)
{
	// greebo: Create a (rotated) quad facing the z axis
	// then rotate it to fit the requested orientation
	// finally translate it to its position.
	const Vector3& normal = _viewRotation.z().getVector3();

	_quads.push_back(ParticleQuad(size, aspect, angle, colour, normal, s0, sWidth));
	_quads.back().transform(_viewRotation);
	_quads.back().translate(origin);
}

void RenderableParticleBunch::pushAnimatedQuads(std::size_t animFrames, const Vector3& particleOrigin, 
												const Vector3& particleVelocity, float size, float aspect, 
												float angle, const Vector4& colour, float particleTimeSecs)
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
	Vector4 nextColour = colour * nextAlpha;

	// The width of a single frame in texture space
	float sWidth = 1.0f / animFrames;

	// Calculate the texture space for each frame and push the quads
	pushQuad(particleOrigin, particleVelocity, size, aspect, angle, curColour, sWidth * curFrame, sWidth);
	pushQuad(particleOrigin, particleVelocity, size, aspect, angle, nextColour, sWidth * nextFrame, sWidth);
}

void RenderableParticleBunch::calculateBounds()
{
	for (Quads::const_iterator i = _quads.begin(); i != _quads.end(); ++i)
	{
		_bounds.includePoint(i->verts[0].vertex);
		_bounds.includePoint(i->verts[1].vertex);
		_bounds.includePoint(i->verts[2].vertex);
		_bounds.includePoint(i->verts[3].vertex);
	}
}

std::string RenderableParticleBunch::getDebugInfo()
{
	return _debugInfo;
}

} // namespace
