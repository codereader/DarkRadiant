#pragma once

#include "isound.h"
#include "math/Vector3.h"
#include "render/RenderableGeometry.h"

namespace entity
{

class RenderableSpeakerRadiiBase :
    public render::RenderableGeometry
{
protected:
    bool _needsUpdate;

    const Vector3& _origin;

    // SoundRadii reference containing min and max radius values
    // (the actual instance resides in the SpeakerNode)
    const SoundRadii& _radii;

protected:
    RenderableSpeakerRadiiBase(const Vector3& origin, const SoundRadii& radii) :
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
    RenderableSpeakerRadiiWireframe(const Vector3& origin, const SoundRadii& radii) :
        RenderableSpeakerRadiiBase(origin, radii)
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
    RenderableSpeakerRadiiFill(const Vector3& origin, const SoundRadii& radii) :
        RenderableSpeakerRadiiBase(origin, radii)
    {}

    void updateGeometry() override;
};

} // namespace
