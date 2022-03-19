#include "RenderableParticleBunch.h"

#include "itextstream.h"
#include "math/pi.h"

#include "string/string.h"

namespace particles
{

RenderableParticleBunch::RenderableParticleBunch(std::size_t index,
	Rand48::result_type randSeed, const IStageDef& stage, const Matrix4& viewRotation,
    const Vector3& direction, const Vector3& entityColour) :
    _index(index),
    _stage(stage),
    _quads(),
    _randSeed(randSeed),
    _distributeParticlesRandomly(_stage.getRandomDistribution()),
    _offset(_stage.getOffset()),
    _viewRotation(viewRotation),
    _direction(direction),
    _entityColour(entityColour)
{
    // Geometry is written in update(), just reserve the space
}

void RenderableParticleBunch::update(std::size_t time)
{
    _bounds = AABB();
    _quads.clear();

    // Length of one cycle (duration + deadtime)
    std::size_t cycleMsec = static_cast<std::size_t>(_stage.getCycleMsec());

    if (cycleMsec == 0)
    {
        return;
    }

    // Reserve enough space for all the particles (non-animated case)
    _quads.reserve(_stage.getCount() * 4);

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

        // Generate the particle renderinfo structure (our working set)
        ParticleRenderInfo particle(i, _random);

        // Calculate the time fraction [0..1]
        particle.timeFraction = static_cast<float>(particleTime) / stageDurationMsec;

        // We need the particle time in seconds for the location/angle integrations
        particle.timeSecs = MS2SEC(particleTime);

        // Calculate particle origin at time t
        calculateOrigin(particle);

        // Get the initial angle value
        particle.angle = _stage.getInitialAngle();

        if (particle.angle == 0)
        {
            // Use random angle
            particle.angle = 360 * static_cast<float>(_random()) / _random.max();
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
        particle.angle += rotFactor * integrate(_stage.getRotationSpeed(), particle.timeSecs);

        // Calculate render colour for this particle
        calculateColour(particle);

        // Consider quad size
        particle.size = _stage.getSize().evaluate(particle.timeFraction);

        // Consider aspect ratio
        particle.aspect = _stage.getAspect().evaluate(particle.timeFraction);

        // Consider animation frames
        particle.animFrames = static_cast<std::size_t>(_stage.getAnimationFrames());

        if (particle.animFrames > 0)
        {
            // Calculate the s coordinates and the resulting particle colour
            calculateAnim(particle);
        }

        // For aimed orientation, we need to override particle height and aspect
        if (_stage.getOrientationType() == IStageDef::ORIENTATION_AIMED)
        {
            pushAimedParticles(particle, stageDurationMsec);
        }
        else
        {
            if (particle.animFrames > 0)
            {
                // Animated, push two crossfaded quads
                pushQuad(particle, particle.curColour, particle.sWidth * particle.curFrame, particle.sWidth);
                pushQuad(particle, particle.nextColour, particle.sWidth * particle.nextFrame, particle.sWidth);
            }
            else
            {
                // Non-animated quad
                pushQuad(particle, particle.colour);
            }
        }
    }
}

void RenderableParticleBunch::addVertexData(std::vector<render::RenderVertex>& vertices, 
    std::vector<unsigned int>& indices, const Matrix4& localToWorld)
{
    if (_quads.empty()) return;

    auto firstIndex = static_cast<unsigned int>(vertices.size());

    auto quadIndex = 0;

    for (const auto& quad : _quads)
    {
        for (auto i = 0; i < 4; ++i)
        {
            auto worldVertex = localToWorld * quad.verts[i].vertex;

            vertices.push_back(render::RenderVertex(
                worldVertex,
                quad.verts[i].normal, 
                quad.verts[i].texcoord, 
                quad.verts[i].colour)
            );
        }

        auto index = firstIndex + quadIndex * 4;

        indices.push_back(index + 0);
        indices.push_back(index + 1);
        indices.push_back(index + 2);

        indices.push_back(index + 0);
        indices.push_back(index + 2);
        indices.push_back(index + 3);

        quadIndex++;
    }
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
    Vector3 view = camera2Object.transformPoint(Vector3(0,0,-1));

    // Project the view vector onto the plane defined by the velocity vector
    Vector3 viewProj = view - vel * view.dot(vel);

    // This is the particle normal in object space (after being oriented such that y || velocity)
    Vector3 z = object2Vel.zCol3();

    // The particle needs to be rotated by this angle around the velocity axis
    double aimedAngle = z.angle(-viewProj);

    // Use the cross to check whether to rotate in negative or positive direction
    if (z.cross(-viewProj).dot(vel) > 0)
    {
        aimedAngle *= -1;
    }

    // Calculate the rotation of the particle normal towards the view vector, around the velocity axis
    Matrix4 vel2aimed = Matrix4::getRotation(vel, aimedAngle);

    // Combine the matrices object2Vel => vel2aimed;
    return vel2aimed.getMultipliedBy(object2Vel);
}

void RenderableParticleBunch::calculateAnim(ParticleRenderInfo& particle)
{
    // At a given time, two particles can be visible at most
    float frameRate = _stage.getAnimationRate();

    // The time interval for cross-fading, fall back to entire duration * 3 for zero animation rates
    float frameIntervalSecs = frameRate > 0 ? 1.0f / frameRate : 3 * _stage.getDuration();

    // Calculate the current frame number, wrap around
    particle.curFrame = static_cast<std::size_t>(floor(particle.timeSecs / frameIntervalSecs)) % particle.animFrames;

    // Wrap next frame around animationFrame count for looping
    particle.nextFrame = (particle.curFrame + 1) % particle.animFrames;

    // Calculate the time within the frame, relative to frame start
    float frameMicrotime = float_mod(particle.timeSecs, frameIntervalSecs);

    // As a fading lasts as long as the entire interval, the alpha gradient is the same as the FPS value
    // The "current" particle is always fading out, the nextFrame is fading in
    float curAlpha = 1.0f - frameRate * frameMicrotime;
    float nextAlpha = frameRate * frameMicrotime;

    particle.curColour = particle.colour * curAlpha;
    particle.nextColour = particle.colour * nextAlpha;

    // The width of a single frame in texture space
    particle.sWidth = 1.0f / particle.animFrames;
}

void RenderableParticleBunch::calculateColour(ParticleRenderInfo& particle)
{
    Vector4 mainColour = !_stage.getUseEntityColour() ?
        _stage.getColour() : Vector4(_entityColour.x(), _entityColour.y(), _entityColour.z(), 1);

    // We start with the stage's standard colour
    particle.colour = mainColour;

    // Consider fade index fraction, which can spawn particles already faded to some extent
    float fadeIndexFraction = _stage.getFadeIndexFraction();

    if (fadeIndexFraction > 0)
    {
        // greebo: The linear fading function goes like this:
        // frac(t) = (startFrac - t) / (startFrac - 1) with t in [0..1]
        // Boundary conditions: frac(1) = 1 and frac(startFrac) = 0

        // Use the particle index as "time", normalised to [0..1]
        // such that particle with higher index start more faded
        float pIdx = static_cast<float>(particle.index) / _stage.getCount();

        // Calculate how much we should be faded already
        float startFrac = 1.0f - fadeIndexFraction;
        float frac = (startFrac - pIdx) / (startFrac - 1.0f);

        // Ignore negative fraction values, this also takes care that only
        // those particles with time >= fadeIndexFraction get faded.
        if (frac > 0)
        {
            particle.colour = lerpColour(particle.colour, _stage.getFadeColour(), frac);
        }
    }

    float fadeInFraction = _stage.getFadeInFraction();

    if (fadeInFraction > 0 && particle.timeFraction <= fadeInFraction)
    {
        particle.colour = lerpColour(_stage.getFadeColour(), mainColour, particle.timeFraction / fadeInFraction);
    }

    float fadeOutFraction = _stage.getFadeOutFraction();
    float fadeOutFractionInverse = 1.0f - fadeOutFraction;

    if (fadeOutFraction > 0 && particle.timeFraction >= fadeOutFractionInverse)
    {
        particle.colour = lerpColour(mainColour, _stage.getFadeColour(), (particle.timeFraction - fadeOutFractionInverse) / fadeOutFraction);
    }
}

void RenderableParticleBunch::calculateOrigin(ParticleRenderInfo& particle)
{
    // Check if the main direction is different to the z axis
    Vector3 dir = _direction.getNormalised();
    Vector3 zDir(0,0,1);

    double deviation = dir.angle(zDir);

    Matrix4 rotation = deviation != 0 ? Matrix4::getRotation(zDir, dir) : Matrix4::getIdentity();

    // Consider offset as starting point
    particle.origin = rotation.transformPoint(_offset);

    switch (_stage.getCustomPathType())
    {
    case IStageDef::PATH_STANDARD: // Standard path calculation
        {
            // Consider particle distribution
            Vector3 distributionOffset = getDistributionOffset(particle, _distributeParticlesRandomly);

            // Add this to the origin
            particle.origin += distributionOffset;

            // Calculate particle direction, pass distribution offset (this is needed for DIRECTION_OUTWARD)
            Vector3 particleDirection = getDirection(particle, rotation, distributionOffset);

            // Consider speed
            particle.origin += particleDirection * integrate(_stage.getSpeed(), particle.timeSecs);
        }
        break;

    case IStageDef::PATH_FLIES:
        {
            // greebo: "Flies" particles are moving on the surface of a sphere of radius <size>
            // The radial and axial speeds are chosen at random (but never 0) and are constant
            // during the lifetime of a particle. Starting position appears to be random,
            // but different to the "distribution sphere" type (i.e. it is not evenly distributed,
            // instead the particles seem to bunch themselves at the poles).

            // Sphere radius
            float radius = _stage.getCustomPathParm(2);

            // Generate starting conditions speed (+/-50%)
            float rand = 2 * particle.rand[0] - 1.0f;
            float radialSpeedFactor = 1.0f + 0.5f * rand * rand;

            // greebo: factor 0.4 is empirical, I measured a few D3 particles for their circulation times
            float radialSpeed = _stage.getCustomPathParm(0) * radialSpeedFactor * 0.4f;

            rand = 2 * particle.rand[1] - 1.0f;
            float axialSpeedFactor = 1.0f + 0.5f * rand * rand;
            float axialSpeed = _stage.getCustomPathParm(1) * axialSpeedFactor * 0.4f;

            float phi0 = 2 * static_cast<float>(math::PI) * particle.rand[2];
            float theta0 = static_cast<float>(math::PI) * particle.rand[3];

            // Calculate angles at the given particleTime
            float phi = phi0 + axialSpeed * particle.timeSecs;
            float theta = theta0 + radialSpeed * particle.timeSecs;

            // Pre-calculate the sin/cos values
            float cosPhi = cos(phi);
            float sinPhi = sin(phi);
            float cosTheta = cos(theta);
            float sinTheta = sin(theta);

            // Move the particle origin
            particle.origin += Vector3(radius * cosTheta * sinPhi, radius * sinTheta * sinPhi, radius * cosPhi);
        }
        break;

    case IStageDef::PATH_HELIX:
        {
            // greebo: Helical movement is describing an elliptic cylinder, its shape is determined by
            // sizeX, sizeY and sizeZ. Particles are spawned randomly on that cylinder surface,
            // their velocities (radial and axial) are also random (both negative and positive
            // velocities are allowed).

            float sizeX = _stage.getCustomPathParm(0);
            float sizeY = _stage.getCustomPathParm(1);
            float sizeZ = _stage.getCustomPathParm(2);

            float radialSpeed = _stage.getCustomPathParm(3) * (2 * particle.rand[0] - 1.0f);
            float axialSpeed = _stage.getCustomPathParm(4) * (2 * particle.rand[1] - 1.0f);

            float phi0 = 2 * static_cast<float>(math::PI) * particle.rand[2];
            float z0 = sizeZ * (2 * particle.rand[3] - 1.0f);

            float sinPhi = sin(phi0 + radialSpeed * particle.timeSecs);
            float cosPhi = cos(phi0 + radialSpeed * particle.timeSecs);

            float x = sizeX * cosPhi;
            float y = sizeY * sinPhi;
            float z = z0 + axialSpeed * particle.timeSecs;

            particle.origin += Vector3(x, y, z);
        }
        break;

    case IStageDef::PATH_ORBIT:
    case IStageDef::PATH_DRIP:
        // These are actually unsupported by the engine ("bad path type")
        rWarning() << "Unsupported path type (drip/orbit)." << std::endl;
        break;

    default:
        // Nothing
        break;
    };

    // Consider gravity
    // if "world" is set, use -z as gravity direction, otherwise use the reverse emitter direction
    Vector3 gravity = _stage.getWorldGravityFlag() ? Vector3(0,0,-1) : -_direction.getNormalised();

    particle.origin += gravity * _stage.getGravity() * particle.timeSecs * particle.timeSecs * 0.5f;
}

Vector3 RenderableParticleBunch::getDirection(ParticleRenderInfo& particle, const Matrix4& rotation, const Vector3& distributionOffset)
{
    switch (_stage.getDirectionType())
    {
    case IStageDef::DIRECTION_CONE:
        {
            // Find a random vector on the sphere surface defined by the cone with apex 2*angle
            float u = particle.rand[3];

            // Scale the variable v such that it takes uniform values in the interval [(1+cos(angle))/2 .. 1]
            float angleRad = _stage.getDirectionParm(0) * static_cast<float>(math::PI) / 180.0f;
            float v0 = (1 + cos(angleRad)) * 0.5f;
            float v1 = 1;

            float v = v0 + particle.rand[4] * (v1 - v0);

            float theta = 2 * static_cast<float>(math::PI) * u;
            float phi = acos(2*v - 1);

            Vector3 endPoint(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));

            // Rotate the vector into the particle's main direction
            endPoint = rotation.transformPoint(endPoint);

            return endPoint.getNormalised();
        }
    case IStageDef::DIRECTION_OUTWARD:
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

Vector3 RenderableParticleBunch::getDistributionOffset(ParticleRenderInfo& particle, bool distributeParticlesRandomly)
{
    switch (_stage.getDistributionType())
    {
        // Rectangular distribution
        case IStageDef::DISTRIBUTION_RECT:
        {
            // Factors to use for the random distribution
            float randX = 1.0f;
            float randY = 1.0f;
            float randZ = 1.0f;

            if (distributeParticlesRandomly)
            {
                // Rectangular spawn zone
                randX = 2 * particle.rand[0] - 1.0f;
                randY = 2 * particle.rand[1] - 1.0f;
                randZ = 2 * particle.rand[2] - 1.0f;
            }

            // If random distribution is off, particles get spawned at <sizex, sizey, sizez>

            return Vector3(randX * _stage.getDistributionParm(0),
                           randY * _stage.getDistributionParm(1),
                           randZ * _stage.getDistributionParm(2));
        }

        case IStageDef::DISTRIBUTION_CYLINDER:
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
                float angle = static_cast<float>(2*math::PI) * particle.rand[0];

                float xPos = cos(angle) * sizeX;
                float yPos = sin(angle) * sizeY;
                float zPos = sizeZ * (2 * particle.rand[1] - 1.0f);

                return Vector3(xPos, yPos, zPos);
            }
            else
            {
                // Random distribution is off, particles get spawned at <sizex, sizey, sizez>
                return Vector3(sizeX, sizeY, sizeZ);
            }
        }

        case IStageDef::DISTRIBUTION_SPHERE:
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
                float u = particle.rand[0];
                float v = particle.rand[1];

                float theta = 2 * static_cast<float>(math::PI) * u;
                float phi = acos(2*v - 1);

                // Take the sqrt(radius) to correct bunching at the center of the sphere
                float r = sqrt(particle.rand[2]);

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

void RenderableParticleBunch::pushQuad(ParticleRenderInfo& particle, const Vector4& colour, float s0, float sWidth)
{
    // greebo: Create a (rotated) quad facing the z axis
    // then rotate it to fit the requested orientation
    // finally translate it to its position.
    const Vector3 normal = _viewRotation.zCol3();

    _quads.push_back(ParticleQuad(particle.size, particle.aspect, particle.angle, colour, normal, s0, sWidth));
    _quads.back().transform(_viewRotation);
    _quads.back().translate(particle.origin);
}

void RenderableParticleBunch::pushAimedParticles(ParticleRenderInfo& particle, std::size_t stageDurationMsec)
{
    int trails = static_cast<int>(_stage.getOrientationParm(0)); // trails
    float aimedTime = _stage.getOrientationParm(1); // time

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

    Vector3 lastOrigin = particle.origin;

    for (int i = 1; i <= numQuads; ++i)
    {
        // Copy over the info of the incoming particle (contains anim info, colour, etc.)
        ParticleRenderInfo aimedParticle = particle;

        // Get the time of the i-th particle in seconds, plus the fraction
        aimedParticle.timeSecs = particle.timeSecs - timeStep * i;
        aimedParticle.timeFraction = SEC2MS(aimedParticle.timeSecs) / stageDurationMsec;

        // Get origin and velocity at that time
        calculateOrigin(aimedParticle);

        // Gotcha: don't bother calculating the actual velocity at the given time, just use the
        // difference vector of the two origins, this is enough to receive the "aimed" direction
        Vector3 velocity = lastOrigin - aimedParticle.origin;

        float height = static_cast<float>(velocity.getLength());

        aimedParticle.aspect = height / (2 * aimedParticle.size);

        // Calculate the vertical texture coordinates
        aimedParticle.tWidth = 1.0f / static_cast<float>(numQuads);
        aimedParticle.t0 = (i - 1) * aimedParticle.tWidth;

        // The matrix is special for each particle. For helix and other path types
        // it's necessary to apply the same matrix to each vertex sharing the same 3D location.

        // Calculate the matrix to orient it towards the viewer
        Matrix4 local2aimed = getAimedMatrix(velocity);

        {
            const Vector3 normal = local2aimed.zCol3();

            // Ignore the angle for aimed orientation
            ParticleQuad curQuad(aimedParticle.size, aimedParticle.aspect, 0,
                                 aimedParticle.colour, normal, 0, 1, aimedParticle.t0, aimedParticle.tWidth);

            // Apply a slight origin correction before rotating them, particles are not centered around 0,0,0 here
            curQuad.translate(Vector3(0, -height*0.5f, 0));
            curQuad.transform(local2aimed);
            curQuad.translate(lastOrigin);

            // Push two quads for animated particles
            if (aimedParticle.animFrames > 0)
            {
                // "Current" quad
                curQuad.assignColour(aimedParticle.curColour);

                // Set the hoirzontal texcoord for the current frame
                curQuad.setHorizTexCoords(aimedParticle.sWidth * aimedParticle.curFrame, aimedParticle.sWidth);

                // Glue the first row of vertices to the last quad, if applicable
                if (i > 1)
                {
                    snapQuads(curQuad, *(_quads.end()-2));
                }

                _quads.push_back(curQuad);

                // "Next" quad, re-use the curQuad structure
                curQuad.assignColour(aimedParticle.nextColour);

                // Set the hoirzontal texcoord for the next frame
                curQuad.setHorizTexCoords(aimedParticle.sWidth * aimedParticle.nextFrame, aimedParticle.sWidth);

                if (i > 1)
                {
                    snapQuads(curQuad, *(_quads.end()-2));
                }

                _quads.push_back(curQuad);
            }
            else
            {
                if (i > 1)
                {
                    snapQuads(curQuad, _quads.back());
                }

                // Non-animated case
                _quads.push_back(curQuad);
            }
        }

        lastOrigin = aimedParticle.origin;
    }
}

void RenderableParticleBunch::snapQuads(ParticleQuad& curQuad, ParticleQuad& prevQuad)
{
    // Take the midpoint
    curQuad.verts[0].vertex = (curQuad.verts[0].vertex + prevQuad.verts[3].vertex) * 0.5f;
    curQuad.verts[1].vertex = (curQuad.verts[1].vertex + prevQuad.verts[2].vertex) * 0.5f;

    // Snap the "previous" vertices to the same spot
    prevQuad.verts[3].vertex = curQuad.verts[0].vertex;
    prevQuad.verts[2].vertex = curQuad.verts[1].vertex;

    // Interpolate the normals too
    curQuad.verts[0].normal = (curQuad.verts[0].normal + prevQuad.verts[3].normal).getNormalised();
    curQuad.verts[1].normal = (curQuad.verts[1].normal + prevQuad.verts[2].normal).getNormalised();

    prevQuad.verts[3].normal = curQuad.verts[0].normal;
    prevQuad.verts[2].normal = curQuad.verts[1].normal;
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

} // namespace
