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

class RenderSpeakerRadii : public OpenGLRenderable {
	const Vector3& m_origin;
	AABB m_aabb_local;
public:
	mutable SoundRadii m_radii;
	static ShaderPtr m_state;

	RenderSpeakerRadii(const Vector3& origin) : m_origin(origin), m_radii() {}
	
	void render(RenderStateFlags state) const;
	const AABB& localAABB();

}; // class RenderSpeakerRadii

} // namespace entity

#endif /*SPEAKERRENDERABLES_H_*/
