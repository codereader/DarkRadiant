#pragma once

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Line.h"
#include "math/Frustum.h"
#include "entitylib.h"
#include "igl.h"
#include "isound.h"

namespace entity
{

/**
 * \brief
 * Renderable speaker radius class.
 *
 * This OpenGLRenderable renders the two spherical radii of a speaker,
 * representing the s_min and s_max values.
 */
class RenderableSpeakerRadii
: public OpenGLRenderable
{
	const Vector3& m_origin;
	AABB m_aabb_local;

    // SoundRadii reference containing min and max radius values
	// (the actual instance resides in the Speaker class)
	const SoundRadii& m_radii;

public:

    /**
     * \brief
     * Construct a RenderableSpeakerRadii with the given origin.
     */
	RenderableSpeakerRadii(const Vector3& origin, const SoundRadii& radii) :
		m_origin(origin),
		m_radii(radii)
    {}

	// Gets the minimum/maximum values to render
	float getMin() const;
	float getMax() const;

	void render(const RenderInfo& info) const;
	const AABB& localAABB();

}; // class RenderSpeakerRadii

} // namespace
