#include "Winding.h"

#include "itextstream.h"
#include <algorithm>
#include "FixedWinding.h"
#include "math/Ray.h"
#include "math/Plane3.h"
#include "texturelib.h"
#include "Brush.h"

namespace {
	struct indexremap_t {
		indexremap_t(std::size_t _x, std::size_t _y, std::size_t _z) :
			x(_x), y(_y), z(_z)
		{}

		std::size_t x, y, z;
	};

	inline indexremap_t indexremap_for_projectionaxis(const ProjectionAxis axis) {
		switch (axis) {
			case eProjectionAxisX:
				return indexremap_t(1, 2, 0);
			case eProjectionAxisY:
				return indexremap_t(2, 0, 1);
			default:
				return indexremap_t(0, 1, 2);
		}
	}
}

void Winding::testSelect(SelectionTest& test, SelectionIntersection& best)
{
	if (empty()) return;

	test.TestPolygon(VertexPointer(&front().vertex, sizeof(WindingVertex)), size(), best);
}

void Winding::updateNormals(const Vector3& normal)
{
	// Copy all normals into the winding vertices
	for (iterator i = begin(); i != end(); ++i)
	{
		i->normal = normal;
	}
}

AABB Winding::aabb() const
{
	AABB returnValue;

	for (const_iterator i = begin(); i != end(); ++i)
	{
		returnValue.includePoint(i->vertex);
	}

	return returnValue;
}

bool Winding::testPlane(const Plane3& plane, bool flipped) const
{
	const int test = (flipped) ? ePlaneBack : ePlaneFront;

	for (const_iterator i = begin(); i != end(); ++i)
	{
		if (test == classifyDistance(plane.distanceToPoint(i->vertex), ON_EPSILON))
		{
			return false;
		}
	}

	return true;
}

BrushSplitType Winding::classifyPlane(const Plane3& plane) const
{
	BrushSplitType split;

	for (const_iterator i = begin(); i != end(); ++i)
	{
		++split.counts[classifyDistance(plane.distanceToPoint(i->vertex), ON_EPSILON)];
	}

	return split;
}

PlaneClassification Winding::classifyDistance(const double distance, const double epsilon)
{
	if (distance > epsilon) {
		return ePlaneFront;
	}

	if (distance < -epsilon) {
		return ePlaneBack;
	}

	return ePlaneOn;
}

bool Winding::planesConcave(const Winding& w1, const Winding& w2, const Plane3& plane1, const Plane3& plane2)
{
	return !w1.testPlane(plane2, false) || !w2.testPlane(plane1, false);
}

std::size_t Winding::findAdjacent(std::size_t face) const
{
	for (std::size_t i = 0; i < size(); ++i)
	{
		ASSERT_MESSAGE((*this)[i].adjacent != brush::c_brush_maxFaces, "edge connectivity data is invalid");
		if ((*this)[i].adjacent == face)
		{
			return i;
		}
	}

	return brush::c_brush_maxFaces;
}

std::size_t Winding::opposite(const std::size_t index, const std::size_t other) const
{
	ASSERT_MESSAGE(index < size() && other < size(), "Winding::opposite: index out of range");

	double dist_best = 0;
	std::size_t index_best = brush::c_brush_maxFaces;

	Ray edge = Ray::createForPoints((*this)[index].vertex, (*this)[other].vertex);

	for (std::size_t i=0; i < size(); ++i)
	{
		if (i == index || i == other) {
			continue;
		}

		auto dist_squared = edge.getSquaredDistance((*this)[i].vertex);

		if (dist_squared > dist_best) {
			dist_best = dist_squared;
			index_best = i;
		}
	}

	return index_best;
}

std::size_t Winding::opposite(std::size_t index) const
{
	return opposite(index, next(index));
}

Vector3 Winding::centroid(const Plane3& plane) const
{
	Vector3 centroid(0,0,0);

	double area2 = 0, x_sum = 0, y_sum = 0;
	const ProjectionAxis axis = projectionaxis_for_normal(plane.normal());
	const indexremap_t remap = indexremap_for_projectionaxis(axis);

	for (std::size_t i = size() - 1, j = 0; j < size(); i = j, ++j)
	{
		const auto ai = (*this)[i].vertex[remap.x]
				* (*this)[j].vertex[remap.y] - (*this)[j].vertex[remap.x]
				* (*this)[i].vertex[remap.y];

		area2 += ai;

		x_sum += ((*this)[j].vertex[remap.x] + (*this)[i].vertex[remap.x]) * ai;
		y_sum += ((*this)[j].vertex[remap.y] + (*this)[i].vertex[remap.y]) * ai;
	}

	centroid[remap.x] = x_sum / (3 * area2);
	centroid[remap.y] = y_sum / (3 * area2);
	{
		Ray ray(Vector3(0, 0, 0), Vector3(0, 0, 0));
		ray.origin[remap.x] = centroid[remap.x];
		ray.origin[remap.y] = centroid[remap.y];
		ray.direction[remap.z] = 1;
		centroid[remap.z] = ray.getDistance(plane);
	}

	return centroid;
}

void Winding::printConnectivity()
{
	for (iterator i = begin(); i != end(); ++i)
	{
		std::size_t vertexIndex = std::distance(begin(), i);
		rMessage() << "vertex: " << vertexIndex
			<< " adjacent: " << i->adjacent << std::endl;
	}
}
