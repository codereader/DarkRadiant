#ifndef RENDERABLES_H_
#define RENDERABLES_H_

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/matrix.h"
#include "entitylib.h"
#include "igl.h"

void light_draw_box_lines(const Vector3& origin, const Vector3 points[8]);

namespace entity {

class RenderLightRadiiBox : public OpenGLRenderable {
	const Vector3& m_origin;
public:
	mutable Vector3 m_points[8];
	static Shader* m_state;

	RenderLightRadiiBox(const Vector3& origin) : m_origin(origin)
	{
	}
	
	void render(RenderStateFlags state) const {
		//draw the bounding box of light based on light_radius key
		if((state & RENDER_FILL) != 0) {
			aabb_draw_flatshade(m_points);
		}
		else {
			aabb_draw_wire(m_points);
		}

  #if 1    //disable if you dont want lines going from the center of the light bbox to the corners
		light_draw_box_lines(m_origin, m_points);
  #endif
	}
}; // class RenderLightRadiiBox

class RenderLightProjection : public OpenGLRenderable {
	const Matrix4& m_projection;
public:
	RenderLightProjection(const Matrix4& projection) : m_projection(projection)
	{
	}
	
	void render(RenderStateFlags state) const {
		Matrix4 unproject(matrix4_full_inverse(m_projection));
		Vector3 points[8];
		aabb_corners(AABB(Vector3(0.5f, 0.5f, 0.5f), Vector3(0.5f, 0.5f, 0.5f)), points);
		points[0] = matrix4_transformed_vector4(unproject, Vector4(points[0], 1)).getProjected();
		points[1] = matrix4_transformed_vector4(unproject, Vector4(points[1], 1)).getProjected();
		points[2] = matrix4_transformed_vector4(unproject, Vector4(points[2], 1)).getProjected();
		points[3] = matrix4_transformed_vector4(unproject, Vector4(points[3], 1)).getProjected();
		points[4] = matrix4_transformed_vector4(unproject, Vector4(points[4], 1)).getProjected();
		points[5] = matrix4_transformed_vector4(unproject, Vector4(points[5], 1)).getProjected();
		points[6] = matrix4_transformed_vector4(unproject, Vector4(points[6], 1)).getProjected();
		points[7] = matrix4_transformed_vector4(unproject, Vector4(points[7], 1)).getProjected();
		Vector4 test1 = matrix4_transformed_vector4(unproject, Vector4(0.5f, 0.5f, 0.5f, 1));
		Vector3 test2 = test1.getProjected();
		aabb_draw_wire(points);
	}
}; // class RenderLightProjection

} // namespace entity

#endif /*RENDERABLES_H_*/
