#pragma once

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Line.h"
#include "math/Frustum.h"
#include "entitylib.h"
#include "igl.h"
#include "isound.h"
#include "render/RenderableGeometry.h"

namespace entity
{

/**
 * \brief
 * Renderable speaker radius class.
 *
 * This OpenGLRenderable renders the two spherical radii of a speaker,
 * representing the s_min and s_max values.
 */
class RenderableSpeakerRadiiWireframe :
    public render::RenderableGeometry
{
private:
    bool _needsUpdate;

	const Vector3& _origin;

    // SoundRadii reference containing min and max radius values
	// (the actual instance resides in the SpeakerNode)
	const SoundRadii& _radii;

public:

    /**
     * \brief
     * Construct an instance with the given origin.
     */
    RenderableSpeakerRadiiWireframe(const Vector3& origin, const SoundRadii& radii) :
		_origin(origin),
		_radii(radii)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

    void updateGeometry() override;

#if 0
	// Gets the minimum/maximum values to render
	float getMin() const;
	float getMax() const;
	void render(const RenderInfo& info) const;
#endif
#if 0
	const AABB& localAABB();
#endif
};

} // namespace
