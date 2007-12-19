#include "FixedWinding.h"

#include "Brush.h"
#include "Winding.h"

namespace {
	inline bool float_is_largest_absolute(double axis, double other) {
		return fabs(axis) > fabs(other);
	}
	
	/// \brief Returns the index of the component of \p v that has the largest absolute value.
	inline int vector3_largest_absolute_component_index(const Vector3& v) {
		return (float_is_largest_absolute(v[1], v[0]))
		    ? (float_is_largest_absolute(v[1], v[2]))
		      ? 1
		      : 2
		    : (float_is_largest_absolute(v[0], v[2]))
		      ? 0
		      : 2;
	}

	/// \brief Returns the infinite line that is the intersection of \p plane and \p other.
	inline DoubleLine plane3_intersect_plane3(const Plane3& plane, const Plane3& other) {
		DoubleLine line;
		line.direction = plane.normal().crossProduct(other.normal());
		
		switch (vector3_largest_absolute_component_index(line.direction)) {
		case 0:
			line.origin.x() = 0;
			line.origin.y() = (-other.dist() * plane.normal().z() - -plane.dist() * other.normal().z()) / line.direction.x();
			line.origin.z() = (-plane.dist() * other.normal().y() - -other.dist() * plane.normal().y()) / line.direction.x();
			break;
		case 1:
			line.origin.x() = (-plane.dist() * other.normal().z() - -other.dist() * plane.normal().z()) / line.direction.y();
			line.origin.y() = 0;
			line.origin.z() = (-other.dist() * plane.normal().x() - -plane.dist() * other.normal().x()) / line.direction.y();
			break;
		case 2:
			line.origin.x() = (-other.dist() * plane.normal().y() - -plane.dist() * other.normal().y()) / line.direction.z();
			line.origin.y() = (-plane.dist() * other.normal().x() - -other.dist() * plane.normal().x()) / line.direction.z();
			line.origin.z() = 0;
			break;
		default:
			break;
		}
	
		return line;
	}
}

void FixedWinding::writeToWinding(Winding& winding) {
	// First, set the target winding to the same size as <self>
	winding.resize(size());
	winding.numpoints = size();
	
	// Now copy stuff from this to the target winding
	for (std::size_t i = 0; i < size(); ++i) {
		winding[i].vertex[0] = (*this)[i].vertex[0];
		winding[i].vertex[1] = (*this)[i].vertex[1];
		winding[i].vertex[2] = (*this)[i].vertex[2];
		winding[i].adjacent = (*this)[i].adjacent;
	}
}

void FixedWinding::createInfinite(const Plane3& plane, double infinity) {
	double max = -infinity;
	int x = -1;
	
	for (int i = 0; i < 3; i++) {
		double d = fabs(plane.normal()[i]);
		if (d > max) {
			x = i;
			max = d;
		}
	}
	
	if (x == -1) {
		globalErrorStream() << "invalid plane\n";
		return;
	}

	Vector3 vup = g_vector3_identity;
	switch (x) {
		case 0:
		case 1:
			vup[2] = 1;
			break;
		case 2:
			vup[0] = 1;
			break;
	}

	vup += plane.normal() * (-vup.dot(plane.normal()));
	vup.normalise();

	Vector3 org = plane.normal() * plane.dist();

	Vector3 vright = vup.crossProduct(plane.normal());

	vup *= infinity;
	vright *= infinity;

	// project a really big  axis aligned box onto the plane

	DoubleLine r1, r2, r3, r4;
	r1.origin = (org - vright) + vup;
	r1.direction = vright.getNormalised();
	push_back(FixedWindingVertex(r1.origin, r1, c_brush_maxFaces));
	
	r2.origin = org + vright + vup;
	r2.direction = (-vup).getNormalised();
	push_back(FixedWindingVertex(r2.origin, r2, c_brush_maxFaces));
	
	r3.origin = (org + vright) - vup;
	r3.direction = (-vright).getNormalised();
	push_back(FixedWindingVertex(r3.origin, r3, c_brush_maxFaces));
	
	r4.origin = (org - vright) - vup;
	r4.direction = vup.getNormalised();
	push_back(FixedWindingVertex(r4.origin, r4, c_brush_maxFaces));
}

/// \brief Clip \p winding which lies on \p plane by \p clipPlane, resulting in \p clipped.
/// If \p winding is completely in front of the plane, \p clipped will be identical to \p winding.  
/// If \p winding is completely in back of the plane, \p clipped will be empty.  
/// If \p winding intersects the plane, the edge of \p clipped which lies on \p clipPlane will store the value of \p adjacent.
void FixedWinding::clip(const Plane3& plane, const Plane3& clipPlane, std::size_t adjacent, FixedWinding& clipped)
{
	if (size() == 0) {
		return; // Degenerate winding, exit
	}

	PlaneClassification classification = Winding::classifyDistance(clipPlane.distanceToPoint(back().vertex), ON_EPSILON);
	PlaneClassification nextClassification;
	
	// for each edge
	for (std::size_t next = 0, i = size() - 1; 
		 next != size(); 
		 i = next, ++next, classification = nextClassification)
	{
		nextClassification = Winding::classifyDistance(clipPlane.distanceToPoint((*this)[next].vertex), ON_EPSILON);
		const FixedWindingVertex& vertex = (*this)[i];

		// if first vertex of edge is ON
		if (classification == ePlaneOn) {
			// append first vertex to output winding
			if (nextClassification == ePlaneBack) {
				// this edge lies on the clip plane
				clipped.push_back(
					FixedWindingVertex(vertex.vertex, plane3_intersect_plane3(plane, clipPlane), adjacent)
				);
			}
			else {
				clipped.push_back(vertex);
			}
			continue;
		}

		// if first vertex of edge is FRONT
		if (classification == ePlaneFront) {
			// add first vertex to output winding
			clipped.push_back(vertex);
		}
		
		// if second vertex of edge is ON
		if (nextClassification == ePlaneOn) {
			continue;
		}
		// else if second vertex of edge is same as first
		else if (nextClassification == classification) {
			continue;
		}
		// else if first vertex of edge is FRONT and there are only two edges
		else if (classification == ePlaneFront && size() == 2) {
			continue;
		}
		// else first vertex is FRONT and second is BACK or vice versa
		else {
			// append intersection point of line and plane to output winding
			Vector3 mid(vertex.edge.intersectPlane(clipPlane));

			if (classification == ePlaneFront) {
				// this edge lies on the clip plane
				clipped.push_back(FixedWindingVertex(mid,
						plane3_intersect_plane3(plane, clipPlane), adjacent));
			} else {
				clipped.push_back(FixedWindingVertex(mid, vertex.edge,
						vertex.adjacent));
			}
		}
	}
}
