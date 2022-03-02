#pragma once

#include "math/Vector3.h"
#include "math/Ray.h"
#include "iselectiontest.h"
#include "render/VertexNCb.h"
#include "render/VertexCb.h"

enum clipcull_t
{
    eClipCullNone,
    eClipCullCW,
    eClipCullCCW,
};

typedef unsigned char ClipResult;

const ClipResult c_CLIP_PASS = 0x00; // 000000
const ClipResult c_CLIP_LT_X = 0x01; // 000001
const ClipResult c_CLIP_GT_X = 0x02; // 000010
const ClipResult c_CLIP_LT_Y = 0x04; // 000100
const ClipResult c_CLIP_GT_Y = 0x08; // 001000
const ClipResult c_CLIP_LT_Z = 0x10; // 010000
const ClipResult c_CLIP_GT_Z = 0x20; // 100000
const ClipResult c_CLIP_FAIL = 0x3F; // 111111

class Segment3D
{
public:
    // Constructor
    Segment3D(const Vector3& _p0, const Vector3& _p1) :
        p0(_p0),
        p1(_p1)
    {}

    Vector3 p0;
    Vector3 p1;
};

inline double triangle_signed_area_XY(const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
    return ((p1[0] - p0[0]) * (p2[1] - p0[1])) - ((p2[0] - p0[0]) * (p1[1] - p0[1]));
}

// get the distance of a point to a segment.
inline Vector3 segment_closest_point_to_point(const Segment3D& segment, const Vector3& point)
{
    Vector3 v = segment.p1 - segment.p0;
    Vector3 w = point - segment.p0;

    double c1 = w.dot(v);
    if (c1 <= 0)
        return segment.p0;

    double c2 = v.dot(v);
    if (c2 <= c1)
        return segment.p1;

    return Vector3(segment.p0 + v * (c1 / c2));
}

// crossing number test for a point in a polygon
// This code is patterned after [Franklin, 2000]
inline bool point_test_polygon_2d(const Vector3& P, const Vector3* start, const Vector3* finish)
{
    std::size_t crossings = 0;

    // loop through all edges of the polygon
    const Vector3* cur = start;
    for (const Vector3* prev = finish - 1; cur != finish; prev = cur, ++cur)
    {    // edge from (*prev) to (*cur)
        if ((((*prev)[1] <= P[1]) && ((*cur)[1] > P[1]))    // an upward crossing
            || (((*prev)[1] > P[1]) && ((*cur)[1] <= P[1])))
        { // a downward crossing
          // compute the actual edge-ray intersect x-coordinate
            double vt = (double)(P[1] - (*prev)[1]) / ((*cur)[1] - (*prev)[1]);
            if (P[0] < (*prev)[0] + vt * ((*cur)[0] - (*prev)[0])) // P[0] < intersect
            {
                ++crossings;   // a valid crossing of y=P[1] right of P[0]
            }
        }
    }
    return (crossings & 0x1) != 0;    // 0 if even (out), and 1 if odd (in)
}

inline void BestPoint(std::size_t count, Vector4 clipped[9], SelectionIntersection& best, clipcull_t cull)
{
    Vector3 normalised[9];

    {
        for (std::size_t i = 0; i < count; ++i)
        {
            normalised[i][0] = clipped[i][0] / clipped[i][3];
            normalised[i][1] = clipped[i][1] / clipped[i][3];
            normalised[i][2] = clipped[i][2] / clipped[i][3];
        }
    }

    if (cull != eClipCullNone && count > 2)
    {
        double signed_area = triangle_signed_area_XY(normalised[0], normalised[1], normalised[2]);

        if ((cull == eClipCullCW && signed_area > 0)
            || (cull == eClipCullCCW && signed_area < 0))
            return;
    }

    if (count == 2)
    {
        Segment3D segment(normalised[0], normalised[1]);
        Vector3 point = segment_closest_point_to_point(segment, Vector3(0, 0, 0));
        best.assignIfCloser(SelectionIntersection(static_cast<float>(point.z()), 0));
    }
    else if (count > 2 && !point_test_polygon_2d(Vector3(0, 0, 0), normalised, normalised + count))
    {
        const Vector3* end = normalised + count;
        const Vector3* current = normalised;
        for (const Vector3* previous = end - 1; current != end; previous = current, ++current)
        {
            Segment3D segment(*previous, *current);
            Vector3 point = segment_closest_point_to_point(segment, Vector3(0, 0, 0));
            double depth = point.z();
            point.z() = 0;
            auto distance = point.getLengthSquared();

            best.assignIfCloser(SelectionIntersection(static_cast<float>(depth), static_cast<float>(distance)));
        }
    }
    else if (count > 2)
    {
        best.assignIfCloser(
            SelectionIntersection(
                static_cast<float>(
                    Ray(Vector3(0, 0, 0), Vector3(0, 0, 1)).getDistance(
                        Plane3(normalised[0], normalised[1], normalised[2])
                    )),
                0
            )
        );
    }
}

namespace
{

class Vector4ClipLT
{
public:
  static bool compare(const Vector4& self, std::size_t index)
  {
    return self[index] < self[3];
  }

  static double scale(const Vector4& self, const Vector4& other, std::size_t index)
  {
    return (self[index] - self[3]) / (other[3] - other[index]);
  }
};

class Vector4ClipGT
{
public:
  static bool compare(const Vector4& self, std::size_t index)
  {
    return self[index] > -self[3];
  }

  static double scale(const Vector4& self, const Vector4& other, std::size_t index)
  {
    return (self[index] + self[3]) / (-other[3] - other[index]);
  }

};

template<typename ClipPlane>
class Vector4ClipPolygon
{
public:
  typedef Vector4* iterator;
  typedef const Vector4* const_iterator;

  static std::size_t apply(const_iterator first, const_iterator last, iterator out, std::size_t index)
  {
    const_iterator next = first, i = last - 1;
    iterator tmp(out);
    bool b0 = ClipPlane::compare(*i, index);
    while(next != last)
    {
      bool b1 = ClipPlane::compare(*next, index);
      if(b0 ^ b1)
      {
        *out = *next - *i;

        double scale = ClipPlane::scale(*i, *out, index);

        (*out)[0] = (*i)[0] + scale*((*out)[0]);
        (*out)[1] = (*i)[1] + scale*((*out)[1]);
        (*out)[2] = (*i)[2] + scale*((*out)[2]);
        (*out)[3] = (*i)[3] + scale*((*out)[3]);

        ++out;
      }

      if(b1)
      {
        *out = *next;
        ++out;
      }

      i = next;
      ++next;
      b0 = b1;
    }

    return out - tmp;
  }
};

#define CLIP_X_LT_W(p) (Vector4ClipLT::compare(p, 0))
#define CLIP_X_GT_W(p) (Vector4ClipGT::compare(p, 0))
#define CLIP_Y_LT_W(p) (Vector4ClipLT::compare(p, 1))
#define CLIP_Y_GT_W(p) (Vector4ClipGT::compare(p, 1))
#define CLIP_Z_LT_W(p) (Vector4ClipLT::compare(p, 2))
#define CLIP_Z_GT_W(p) (Vector4ClipGT::compare(p, 2))

inline ClipResult homogenous_clip_point(const Vector4& clipped)
{
    ClipResult result = c_CLIP_FAIL;

    if (CLIP_X_LT_W(clipped)) result &= ~c_CLIP_LT_X; // X < W
    if (CLIP_X_GT_W(clipped)) result &= ~c_CLIP_GT_X; // X > -W
    if (CLIP_Y_LT_W(clipped)) result &= ~c_CLIP_LT_Y; // Y < W
    if (CLIP_Y_GT_W(clipped)) result &= ~c_CLIP_GT_Y; // Y > -W
    if (CLIP_Z_LT_W(clipped)) result &= ~c_CLIP_LT_Z; // Z < W
    if (CLIP_Z_GT_W(clipped)) result &= ~c_CLIP_GT_Z; // Z > -W

    return result;
}

inline std::size_t homogenous_clip_line(Vector4 clipped[2])
{
    const Vector4& p0 = clipped[0];
    const Vector4& p1 = clipped[1];

    // early out
    {
        ClipResult mask0 = homogenous_clip_point(clipped[0]);
        ClipResult mask1 = homogenous_clip_point(clipped[1]);

        if ((mask0 | mask1) == c_CLIP_PASS) // both points passed all planes
        {
            return 2;
        }

        if (mask0 & mask1) // both points failed any one plane
        {
            return 0;
        }
    }

    {
        const bool index = CLIP_X_LT_W(p0);

        if (index ^ CLIP_X_LT_W(p1))
        {
            Vector4 clip(p1 - p0);

            double scale = (p0[0] - p0[3]) / (clip[3] - clip[0]);

            clip[0] = p0[0] + scale * clip[0];
            clip[1] = p0[1] + scale * clip[1];
            clip[2] = p0[2] + scale * clip[2];
            clip[3] = p0[3] + scale * clip[3];

            clipped[index] = clip;
        }
        else if(index == 0)
        {
            return 0;
        }
    }

    {
        const bool index = CLIP_X_GT_W(p0);

        if (index ^ CLIP_X_GT_W(p1))
        {
            Vector4 clip(p1 - p0);

            double scale = (p0[0] + p0[3]) / (-clip[3] - clip[0]);

            clip[0] = p0[0] + scale * clip[0];
            clip[1] = p0[1] + scale * clip[1];
            clip[2] = p0[2] + scale * clip[2];
            clip[3] = p0[3] + scale * clip[3];

            clipped[index] = clip;
        }
        else if(index == 0)
        {
            return 0;
        }
    }

    {
        const bool index = CLIP_Y_LT_W(p0);

        if (index ^ CLIP_Y_LT_W(p1))
        {
            Vector4 clip(p1 - p0);

            double scale = (p0[1] - p0[3]) / (clip[3] - clip[1]);

            clip[0] = p0[0] + scale * clip[0];
            clip[1] = p0[1] + scale * clip[1];
            clip[2] = p0[2] + scale * clip[2];
            clip[3] = p0[3] + scale * clip[3];

            clipped[index] = clip;
        }
        else if (index == 0)
        {
            return 0;
        }
    }

    {
        const bool index = CLIP_Y_GT_W(p0);

        if (index ^ CLIP_Y_GT_W(p1))
        {
            Vector4 clip(p1 - p0);

            double scale = (p0[1] + p0[3]) / (-clip[3] - clip[1]);

            clip[0] = p0[0] + scale * clip[0];
            clip[1] = p0[1] + scale * clip[1];
            clip[2] = p0[2] + scale * clip[2];
            clip[3] = p0[3] + scale * clip[3];

            clipped[index] = clip;
        }
        else if (index == 0)
        {
            return 0;
        }
    }

    {
        const bool index = CLIP_Z_LT_W(p0);

        if (index ^ CLIP_Z_LT_W(p1))
        {
            Vector4 clip(p1 - p0);

            double scale = (p0[2] - p0[3]) / (clip[3] - clip[2]);

            clip[0] = p0[0] + scale * clip[0];
            clip[1] = p0[1] + scale * clip[1];
            clip[2] = p0[2] + scale * clip[2];
            clip[3] = p0[3] + scale * clip[3];

            clipped[index] = clip;
        }
        else if (index == 0)
        {
            return 0;
        }
    }

    {
        const bool index = CLIP_Z_GT_W(p0);

        if (index ^ CLIP_Z_GT_W(p1))
        {
            Vector4 clip(p1 - p0);

            double scale = (p0[2] + p0[3]) / (-clip[3] - clip[2]);

            clip[0] = p0[0] + scale * clip[0];
            clip[1] = p0[1] + scale * clip[1];
            clip[2] = p0[2] + scale * clip[2];
            clip[3] = p0[3] + scale * clip[3];

            clipped[index] = clip;
        }
        else if (index == 0)
        {
            return 0;
        }
    }

    return 2;
}

inline std::size_t homogenous_clip_triangle(Vector4 clipped[9])
{
    Vector4 buffer[9];
    std::size_t count = 3;

    count = Vector4ClipPolygon< Vector4ClipLT >::apply(clipped, clipped + count, buffer, 0);
    count = Vector4ClipPolygon< Vector4ClipGT >::apply(buffer, buffer + count, clipped, 0);
    count = Vector4ClipPolygon< Vector4ClipLT >::apply(clipped, clipped + count, buffer, 1);
    count = Vector4ClipPolygon< Vector4ClipGT >::apply(buffer, buffer + count, clipped, 1);
    count = Vector4ClipPolygon< Vector4ClipLT >::apply(clipped, clipped + count, buffer, 2);

    return Vector4ClipPolygon< Vector4ClipGT >::apply(buffer, buffer + count, clipped, 2);
}

} // namespace


/**
 * \brief Clip point by this canonical matrix and store the result in clipped.
 *
 * \return Bitmask indicating which clip-planes the point was outside.
 */
inline ClipResult clipPoint(const Matrix4& matrix, const Vector3& point,
                            Vector4& clipped)
{
	clipped[0] = point[0];
	clipped[1] = point[1];
	clipped[2] = point[2];
	clipped[3] = 1;

	clipped = matrix.transform(clipped);

	return homogenous_clip_point(clipped);
}

/**
 * \brief Transform and clip the line formed by p0, p1 by this canonical
 * matrix.
 *
 * Stores the resulting line in clipped.
 *
 * \returns the number of points in the resulting line.
 */
inline std::size_t clipLine(const Matrix4& matrix, const Vector3& p0,
                            const Vector3& p1, Vector4 clipped[2])
{
	clipped[0][0] = p0[0];
	clipped[0][1] = p0[1];
	clipped[0][2] = p0[2];
	clipped[0][3] = 1;
	clipped[1][0] = p1[0];
	clipped[1][1] = p1[1];
	clipped[1][2] = p1[2];
	clipped[1][3] = 1;

	clipped[0] = matrix.transform(clipped[0]);
	clipped[1] = matrix.transform(clipped[1]);

	return homogenous_clip_line(clipped);
}

inline void LineStrip_BestPoint(const Matrix4& local2view, const Vertex3* vertices, const std::size_t size, SelectionIntersection& best)
{
    Vector4 clipped[2];
    for (std::size_t i = 0; (i + 1) < size; ++i)
    {
        const std::size_t count = clipLine(local2view, vertices[i], vertices[i + 1], clipped);
        BestPoint(count, clipped, best, eClipCullNone);
    }
}

inline void LineLoop_BestPoint(const Matrix4& local2view, const Vertex3* vertices, const std::size_t size, SelectionIntersection& best)
{
    Vector4 clipped[2];
    for (std::size_t i = 0; i < size; ++i)
    {
        const std::size_t count = clipLine(local2view, vertices[i], vertices[(i + 1) % size], clipped);
        BestPoint(count, clipped, best, eClipCullNone);
    }
}

inline void Line_BestPoint(const Matrix4& local2view, const Vertex3 vertices[2], SelectionIntersection& best)
{
    Vector4 clipped[2];
    const std::size_t count = clipLine(local2view, vertices[0], vertices[1], clipped);
    BestPoint(count, clipped, best, eClipCullNone);
}

/**
 * \brief Use the given Matrix to transform and clips the triangle formed by p0, p1,
 * p2.
 *
 * Stores the resulting polygon in clipped.
 * Returns the number of points in the resulting polygon.
 */
inline std::size_t clipTriangle(const Matrix4& matrix, const Vector3& p0,
                                const Vector3& p1, const Vector3& p2,
                                Vector4 clipped[9])
{
	clipped[0][0] = p0[0];
	clipped[0][1] = p0[1];
	clipped[0][2] = p0[2];
	clipped[0][3] = 1;
	clipped[1][0] = p1[0];
	clipped[1][1] = p1[1];
	clipped[1][2] = p1[2];
	clipped[1][3] = 1;
	clipped[2][0] = p2[0];
	clipped[2][1] = p2[1];
	clipped[2][2] = p2[2];
	clipped[2][3] = 1;

	clipped[0] = matrix.transform(clipped[0]);
	clipped[1] = matrix.transform(clipped[1]);
	clipped[2] = matrix.transform(clipped[2]);

	return homogenous_clip_triangle(clipped);
}

inline void Circle_BestPoint(const Matrix4& local2view, clipcull_t cull, const Vertex3* vertices, const std::size_t size, SelectionIntersection& best)
{
    Vector4 clipped[9];
    for (std::size_t i = 0; i < size; ++i)
    {
        const std::size_t count = clipTriangle(
            local2view, g_vector3_identity, vertices[i],
            vertices[(i + 1) % size], clipped
        );
        BestPoint(count, clipped, best, cull);
    }
}

inline void Quad_BestPoint(const Matrix4& local2view, clipcull_t cull, const Vertex3* vertices, SelectionIntersection& best)
{
    Vector4 clipped[9];
    {
        const std::size_t count = clipTriangle(local2view, vertices[0], vertices[1], vertices[3], clipped);
        BestPoint(count, clipped, best, cull);
    }
    {
        const std::size_t count = clipTriangle(local2view, vertices[1], vertices[2], vertices[3], clipped);
        BestPoint(count, clipped, best, cull);
    }
}

inline void Triangles_BestPoint(const Matrix4& local2view, clipcull_t cull, const Vertex3* first, const Vertex3* last, SelectionIntersection& best)
{
    for (auto x(first), y(first + 1), z(first + 2); x != last; x += 3, y += 3, z += 3)
    {
        Vector4 clipped[9];
        BestPoint(
            clipTriangle(local2view, *x, *y, *z, clipped),
            clipped, best, cull
        );
    }
}
