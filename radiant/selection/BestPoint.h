#ifndef BESTPOINT_H_
#define BESTPOINT_H_

#include "math/Vector3.h"
#include "selectable.h"
#include "Manipulatables.h"

/* greebo: These are functions that are needed for selection test (which points are nearest to what and something) 
 */

enum clipcull_t
{
  eClipCullNone,
  eClipCullCW,
  eClipCullCCW,
};

class Segment3D {
  	typedef Vector3 point_type;  
public:
	// Constructor
  	Segment3D(const point_type& _p0, const point_type& _p1): p0(_p0), p1(_p1) {}

	point_type p0, p1;
};

typedef Vector3 Point3D;

inline double triangle_signed_area_XY(const Vector3& p0, const Vector3& p1, const Vector3& p2) {
  return ((p1[0] - p0[0]) * (p2[1] - p0[1])) - ((p2[0] - p0[0]) * (p1[1] - p0[1]));
}

// get the distance of a point to a segment.
Point3D segment_closest_point_to_point(const Segment3D& segment, const Point3D& point);

typedef Vector3 point_t;
typedef const Vector3* point_iterator_t;

// crossing number test for a point in a polygon
// This code is patterned after [Franklin, 2000]
bool point_test_polygon_2d( const point_t& P, point_iterator_t start, point_iterator_t finish );

void BestPoint(std::size_t count, Vector4 clipped[9], SelectionIntersection& best, clipcull_t cull);

void LineStrip_BestPoint(const Matrix4& local2view, const PointVertex* vertices, const std::size_t size, SelectionIntersection& best);
void LineLoop_BestPoint(const Matrix4& local2view, const PointVertex* vertices, const std::size_t size, SelectionIntersection& best);
void Line_BestPoint(const Matrix4& local2view, const PointVertex vertices[2], SelectionIntersection& best);

void Circle_BestPoint(const Matrix4& local2view, clipcull_t cull, const PointVertex* vertices, const std::size_t size, SelectionIntersection& best);
void Quad_BestPoint(const Matrix4& local2view, clipcull_t cull, const PointVertex* vertices, SelectionIntersection& best);

typedef FlatShadedVertex* FlatShadedVertexIterator;
void Triangles_BestPoint(const Matrix4& local2view, clipcull_t cull, FlatShadedVertexIterator first, FlatShadedVertexIterator last, SelectionIntersection& best);

#endif /*BESTPOINT_H_*/
