#ifndef RENDERABLES_H_
#define RENDERABLES_H_

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/matrix.h"
#include "math/line.h"
#include "math/frustum.h"
#include "entitylib.h"
#include "igl.h"

void light_draw_box_lines(const Vector3& origin, const Vector3 points[8]);

namespace entity {

class RenderLightRadiiBox : public OpenGLRenderable {
	const Vector3& m_origin;
public:
	mutable Vector3 m_points[8];
	static ShaderPtr m_state;

	RenderLightRadiiBox(const Vector3& origin) : m_origin(origin) {}
	
	void render(RenderStateFlags state) const;
}; // class RenderLightRadiiBox

class RenderLightProjection : public OpenGLRenderable {
	const Vector3& _origin;
	const Vector3& _start;
	const Frustum& _frustum;
public:
	RenderLightProjection(const Vector3& origin, const Vector3& start, const Frustum& frustum);
	
	// greebo: Renders the light cone of a projected light (may also be a frustum, when light_start / light_end are set)
	void render(RenderStateFlags state) const;
}; // class RenderLightProjection

} // namespace entity

#endif /*RENDERABLES_H_*/
