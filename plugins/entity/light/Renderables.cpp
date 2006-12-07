#include "Renderables.h"

void light_draw_box_lines(const Vector3& origin, const Vector3 points[8]) {
	//draw lines from the center of the bbox to the corners
	glBegin(GL_LINES);

	glVertex3fv(origin);
	glVertex3fv(points[1]);

	glVertex3fv(origin);
	glVertex3fv(points[5]);

	glVertex3fv(origin);
	glVertex3fv(points[2]);

	glVertex3fv(origin);
	glVertex3fv(points[6]);

	glVertex3fv(origin);
	glVertex3fv(points[0]);

	glVertex3fv(origin);
	glVertex3fv(points[4]);

	glVertex3fv(origin);
	glVertex3fv(points[3]);

	glVertex3fv(origin);
	glVertex3fv(points[7]);

	glEnd();
}

namespace entity 
{

void RenderLightRadiiBox::render(RenderStateFlags state) const {
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

RenderLightProjection::RenderLightProjection(const Vector3& origin, const Vector3& start, const Frustum& frustum) 
  :	_origin(origin),
  	_start(start),
	_frustum(frustum)
{
}

void RenderLightProjection::render(RenderStateFlags state) const {
		
	// greebo: These four define the base area and are always needed to draw the light
	// Note the minus sign before intersectPlanes
	Vector3 bottomUpRight = -intersectPlanes(_frustum.left, _frustum.top, _frustum.back);
	Vector3 bottomDownRight = -intersectPlanes(_frustum.left, _frustum.bottom, _frustum.back);
	Vector3 bottomUpLeft = -intersectPlanes(_frustum.right, _frustum.top, _frustum.back);
	Vector3 bottomDownLeft = -intersectPlanes(_frustum.right, _frustum.bottom, _frustum.back);
	
	// The planes of the frustum are measured at world 0,0,0 so we have to position the intersection points relative to the light origin
	bottomUpRight += _origin;
	bottomDownRight += _origin;
	bottomUpLeft += _origin;
	bottomDownLeft += _origin;
	
	if (_start != Vector3(0,0,0)) {
		// Calculate the vertices defining the top area 
		// Again, note the minus sign
		Vector3 topUpRight = -intersectPlanes(_frustum.left, _frustum.top, _frustum.front);
		Vector3 topDownRight = -intersectPlanes(_frustum.left, _frustum.bottom, _frustum.front);
		Vector3 topUpLeft = -intersectPlanes(_frustum.right, _frustum.top, _frustum.front);
		Vector3 topDownLeft = -intersectPlanes(_frustum.right, _frustum.bottom, _frustum.front);
		
		topUpRight += _origin;
		topDownRight += _origin;
		topUpLeft += _origin;
		topDownLeft += _origin;
		
		Vector3 frustum[8] = { topUpRight, topDownRight, topDownLeft, topUpLeft, 
							   bottomUpRight, bottomDownRight, bottomDownLeft, bottomUpLeft };
		drawFrustum(frustum);
	}
	else {
		// no light_start, just use the top vertex (doesn't need to be mirrored)
		Vector3 top = intersectPlanes(_frustum.left, _frustum.right, _frustum.top);
		top += _origin;
		
		Vector3 pyramid[5] = { top, bottomUpRight, bottomDownRight, bottomDownLeft, bottomUpLeft };
		drawPyramid(pyramid);
	}
}
	
} // namespace entity