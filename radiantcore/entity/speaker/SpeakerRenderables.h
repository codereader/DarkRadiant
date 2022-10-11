#pragma once

#include "isound.h"
#include "math/Vector3.h"
#include "render/RenderableGeometry.h"

class IEntityNode;

namespace entity
{

class RenderableSpeakerRadiiBase :
    public render::RenderableGeometry
{
protected:
    bool _needsUpdate;

    const IEntityNode& _entity;

    const Vector3& _origin;

    // SoundRadii reference containing min and max radius values
    // (the actual instance resides in the SpeakerNode)
    const SoundRadii& _radii;

protected:
    RenderableSpeakerRadiiBase(const IEntityNode& entity, const Vector3& origin, const SoundRadii& radii) :
        _entity(entity),
        _origin(origin),
        _radii(radii)
    {}

public:
    void queueUpdate()
    {
        _needsUpdate = true;
    }
};

/**
 * \brief
 * Renderable speaker radius class (wireframe mode).
 * Draws 3 axis-aligned circles per radius.
 */
class RenderableSpeakerRadiiWireframe :
    public RenderableSpeakerRadiiBase
{
public:
    // Construct an instance with the given origin and radius.
    RenderableSpeakerRadiiWireframe(const IEntityNode& entity, const Vector3& origin, const SoundRadii& radii) :
        RenderableSpeakerRadiiBase(entity, origin, radii)
    {}

    void updateGeometry() override;
};

/**
 * \brief
 * Renderable speaker radius class (camera mode).
 * Draws a quad-based sphere with a fixed number of subdivisions.
 */
class RenderableSpeakerRadiiFill :
    public RenderableSpeakerRadiiBase
{
public:
    // Construct an instance with the given origin and radius.
    RenderableSpeakerRadiiFill(const IEntityNode& entity, const Vector3& origin, const SoundRadii& radii) :
        RenderableSpeakerRadiiBase(entity, origin, radii)
    {}

    void updateGeometry() override;

private:
    void generateSphereVertices(std::vector<render::RenderVertex>& vertices, double radius);
};

} // namespace
