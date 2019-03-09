#include "Renderables.h"

void light_draw_box_lines(const Vector3& origin, const Vector3 points[8]) {
	//draw lines from the center of the bbox to the corners
	glBegin(GL_LINES);

	glVertex3dv(origin);
	glVertex3dv(points[1]);

	glVertex3dv(origin);
	glVertex3dv(points[5]);

	glVertex3dv(origin);
	glVertex3dv(points[2]);

	glVertex3dv(origin);
	glVertex3dv(points[6]);

	glVertex3dv(origin);
	glVertex3dv(points[0]);

	glVertex3dv(origin);
	glVertex3dv(points[4]);

	glVertex3dv(origin);
	glVertex3dv(points[3]);

	glVertex3dv(origin);
	glVertex3dv(points[7]);

	glEnd();
}

namespace entity
{

void RenderLightRadiiBox::render(const RenderInfo& info) const {
	//draw the bounding box of light based on light_radius key
	aabb_draw_wire(m_points);

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

// #define this to display frustum normals
//#define DRAW_LIGHT_FRUSTUM_NORMALS

#ifdef DRAW_LIGHT_FRUSTUM_NORMALS
namespace
{

// Draw a normal on the plane given by the four points a,b,c,d
void drawCenterNormal(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d,
	const Vector3& direction, float length)
{
	Vector3 middle = (a + b + c + d) / 4;
	
	glBegin(GL_LINES);

	glVertex3dv(middle);
	glVertex3dv(middle + direction * length);

	glEnd();
}

}
#endif

void RenderLightProjection::render(const RenderInfo& info) const {

	// greebo: These four define the base area and are always needed to draw the light
	Vector3 backUpperLeft = Plane3::intersect(_frustum.left, _frustum.top, _frustum.back);
	Vector3 backLowerLeft = Plane3::intersect(_frustum.left, _frustum.bottom, _frustum.back);
	Vector3 backUpperRight = Plane3::intersect(_frustum.right, _frustum.top, _frustum.back);
	Vector3 backLowerRight = Plane3::intersect(_frustum.right, _frustum.bottom, _frustum.back);

	// Move all points to world space
	backUpperLeft += _origin;
	backLowerLeft += _origin;
	backUpperRight += _origin;
	backLowerRight += _origin;

	if (_start != Vector3(0,0,0))
	{
		// Calculate the vertices defining the top area
		Vector3 frontUpperLeft = Plane3::intersect(_frustum.left, _frustum.top, _frustum.front);
		Vector3 frontLowerLeft = Plane3::intersect(_frustum.left, _frustum.bottom, _frustum.front);
		Vector3 frontUpperRight = Plane3::intersect(_frustum.right, _frustum.top, _frustum.front);
		Vector3 frontLowerRight = Plane3::intersect(_frustum.right, _frustum.bottom, _frustum.front);

		frontUpperLeft += _origin;
		frontLowerLeft += _origin;
		frontUpperRight += _origin;
		frontLowerRight += _origin;

		Vector3 frustum[8] = { frontUpperLeft, frontLowerLeft, frontLowerRight, frontUpperRight,
							   backUpperLeft, backLowerLeft, backLowerRight, backUpperRight };
		drawFrustum(frustum);

#ifdef DRAW_LIGHT_FRUSTUM_NORMALS
		float length = 20;
		
		drawCenterNormal(backUpperLeft, frontUpperLeft, backLowerLeft, frontLowerLeft, _frustum.left.normal(), length);
		drawCenterNormal(backLowerLeft, frontLowerLeft, frontLowerRight, backLowerRight, _frustum.bottom.normal(), length);
		drawCenterNormal(frontUpperRight, backUpperRight, backLowerRight, frontLowerRight, _frustum.right.normal(), length);
		drawCenterNormal(backUpperLeft, backUpperRight, frontUpperRight, frontUpperLeft, _frustum.top.normal(), length);
		drawCenterNormal(frontUpperLeft, frontLowerLeft, frontLowerRight, frontUpperRight, _frustum.front.normal(), length);
		drawCenterNormal(backUpperLeft, backLowerLeft, backLowerRight, backUpperRight, _frustum.back.normal(), length);
#endif
	}
	else {
		// no light_start, just use the top vertex (doesn't need to be mirrored)
		Vector3 top = Plane3::intersect(_frustum.left, _frustum.right, _frustum.top);
		top += _origin;

		Vector3 pyramid[5] = { top, backUpperLeft, backLowerLeft, backLowerRight, backUpperRight };
		drawPyramid(pyramid);
	}
}

} // namespace entity
