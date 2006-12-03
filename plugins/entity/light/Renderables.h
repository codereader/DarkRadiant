#ifndef RENDERABLES_H_
#define RENDERABLES_H_

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/matrix.h"
#include "math/line.h"
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
	const Vector3& _origin;
	const Vector3& _target;
	const Vector3& _right;
	const Vector3& _up;
public:
	RenderLightProjection(const Matrix4& projection, const Vector3& origin, const Vector3& target,
						  const Vector3& right, const Vector3& up) 
	  :	m_projection(projection),
		_origin(origin),
		_target(target),
		_right(right),
		_up(up)
	{
	}
	
	void render(RenderStateFlags state) const {
		Vector3 basePoint = _origin + _target;
		Vector3 baseRight = basePoint + _right;
		Vector3 baseUp = basePoint + _up;
		
		Vector3 edgeRightDirection = (_right*(-1)).crossProduct(_origin - baseRight).getNormalised();
		
		// The lines going through the edges of the base area
		Ray edgeRight(baseRight, edgeRightDirection);
		Ray edgeUp(baseUp, (_up*(-1)).crossProduct(_origin - baseUp).getNormalised());
		
		// Calculate the corners of the base area  
		Vector3 cornerUpRight = edgeRight.getIntersection(edgeUp);
		Vector3 cornerDownLeft = basePoint*2 - cornerUpRight;
		
		Ray edgeLeft(cornerDownLeft, edgeRightDirection);
		Vector3 cornerUpLeft = edgeLeft.getIntersection(edgeUp);
		Vector3 cornerDownRight = basePoint*2 - cornerUpLeft;
		
		Vector3 pyramid[5] = { Vector3(0,0,0), cornerUpRight - _origin, cornerUpLeft - _origin, 
										       cornerDownLeft - _origin, cornerDownRight - _origin}; 
		
		draw_pyramid(pyramid);
	}
}; // class RenderLightProjection

} // namespace entity

#endif /*RENDERABLES_H_*/
