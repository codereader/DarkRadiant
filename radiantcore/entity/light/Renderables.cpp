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

/* greebo: draws a frustum defined by 8 vertices
 * points[0] to points[3] define the top area vertices (clockwise starting from the "upper right" corner)
 * points[4] to points[7] define the base rectangle (clockwise starting from the "upper right" corner)
 */
inline void drawFrustum(const Vector3 points[8])
{
  typedef unsigned int index_t;
  index_t indices[24] = {
    0, 4, // top up right to bottom up right
    1, 5, // top down right to bottom down right
    2, 6, // top down left to bottom down left
    3, 7, // top up left to bottom up left

    0, 1, // top up right to top down right
    1, 2, // top down right to top down left
    2, 3, // top down left to top up left
    3, 0, // top up left to top up right

    4, 5, // bottom up right to bottom down right
    5, 6, // bottom down right to bottom down left
    6, 7, // bottom down left to bottom up left
    7, 4, // bottom up left to bottom up right
  };
  glVertexPointer(3, GL_DOUBLE, 0, points);
  glDrawElements(GL_LINES, sizeof(indices)/sizeof(index_t), GL_UNSIGNED_INT, indices);
}

/* greebo: draws a pyramid defined by 5 vertices
 * points[0] is the top of the pyramid
 * points[1] to points[4] is the base rectangle
 */
inline void drawPyramid(const Vector3 points[5])
{
  typedef unsigned int index_t;
  index_t indices[16] = {
    0, 1, // top to first
    0, 2, // top to second
    0, 3, // top to third
    0, 4, // top to fourth
    1, 2, // first to second
    2, 3, // second to third
    3, 4, // third to second
    4, 1, // fourth to first
  };
  glVertexPointer(3, GL_DOUBLE, 0, points);
  glDrawElements(GL_LINES, sizeof(indices)/sizeof(index_t), GL_UNSIGNED_INT, indices);
}

void RenderLightProjection::render(const RenderInfo& info) const
{
    // greebo: These four define the base area and are always needed to draw the light
    Vector3 backUpperLeft = _frustum.getCornerPoint(Frustum::BACK, Frustum::TOP_LEFT);
    Vector3 backLowerLeft = _frustum.getCornerPoint(Frustum::BACK, Frustum::BOTTOM_LEFT);
    Vector3 backUpperRight = _frustum.getCornerPoint(Frustum::BACK, Frustum::TOP_RIGHT);
    Vector3 backLowerRight = _frustum.getCornerPoint(Frustum::BACK, Frustum::BOTTOM_RIGHT);

    // Move all points to world space
    backUpperLeft += _origin;
    backLowerLeft += _origin;
    backUpperRight += _origin;
    backLowerRight += _origin;

    if (_start != Vector3(0,0,0))
    {
        // Calculate the vertices defining the top area
        Vector3 frontUpperLeft = _frustum.getCornerPoint(Frustum::FRONT, Frustum::TOP_LEFT);
        Vector3 frontLowerLeft = _frustum.getCornerPoint(Frustum::FRONT, Frustum::BOTTOM_LEFT);
        Vector3 frontUpperRight = _frustum.getCornerPoint(Frustum::FRONT, Frustum::TOP_RIGHT);
        Vector3 frontLowerRight = _frustum.getCornerPoint(Frustum::FRONT, Frustum::BOTTOM_RIGHT);

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
