#ifndef SPEAKERRENDERABLES_H_
#define SPEAKERRENDERABLES_H_

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/matrix.h"
#include "math/line.h"
#include "math/frustum.h"
#include "entitylib.h"
#include "igl.h"
#include "isound.h"

// the drawing functions
void sphereDrawFill(const Vector3& origin, float radius, int sides);
void sphereDrawWire(const Vector3& origin, float radius, int sides);
void speakerDrawRadiiWire(const Vector3& origin, const float envelope[2]);
void speakerDrawRadiiFill(const Vector3& origin, const float envelope[2]);

namespace entity {

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

    // SoundRadii object containing min and max radius values
	SoundRadii m_radii;

public:

    /**
     * \brief
     * Construct a RenderableSpeakerRadii with the given origin.
     */
	RenderableSpeakerRadii(const Vector3& origin) 
    : m_origin(origin)
    { }

    /**
     * \brief
     * Set the minimum radius to render.
     */
    void setMin(float min, bool inMetres = false);

    /**
     * \brief
     * Set the maximum radius to render.
     */
    void setMax(float max, bool inMetres = false);
	
	void render(const RenderInfo& info) const;
	const AABB& localAABB();

}; // class RenderSpeakerRadii

} // namespace entity

#endif /*SPEAKERRENDERABLES_H_*/
