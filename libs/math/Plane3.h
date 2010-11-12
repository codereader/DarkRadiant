#ifndef PLANE3_H_
#define PLANE3_H_

/* greebo: A plane in 3D space can be represented by a point and a normal vector.
 *
 * It is sufficient to specify four numbers to fully describe the plane: the three
 * components of the normal vector (x,y,z) and the dot product of the normal and any point of this plane.
 * (basically this is the "height" at which the plane intersects the z-axis)
 *
 * There are several constructors available: one requires all four number be passed directly,
 * the second requires the normal vector and the distance <dist> to be passed, the third and fourth
 * requires a set of three points that define the plane.
 *
 * Note: the plane numbers are stored in double precision.
 * Note: the constructor requiring three points does NOT check if two or more points are equal.
 * Note: two planes are considered equal when the difference of their normals and distances are below an epsilon.
 */

#include "FloatTools.h"
#include "Vector3.h"

namespace
{
	// Some constants for "equality" check.
	const double EPSILON_NORMAL = 0.0001f;
	const double EPSILON_DIST = 0.02;
}

class Plane3
{
private:
	Vector3 _normal; // normal vector
	double _dist;		// distance

public:
	// Constructor with no arguments
	Plane3() {}

	// Constructor which expects four numbers, the first three are the components of the normal vector.
  	Plane3(double nx, double ny, double nz, double dist) :
		_normal(nx, ny, nz),
		_dist(dist)
	{}

  	// Construct a plane from any BasicVector3 and the distance <dist>
	template<typename Element>
  	Plane3(const BasicVector3<Element>& normal, double dist) :
		_normal(normal),
		_dist(dist)
  	{}

 	// Construct a plane from three points <p0>, <p1> and <p2>
	template<typename Element>
	Plane3(const BasicVector3<Element>& p0, const BasicVector3<Element>& p1, const BasicVector3<Element>& p2) :
		_normal((p1 - p0).crossProduct(p2 - p0).getNormalised()),
		_dist(p0.dot(_normal))
	{}

	// Construct a plane from three points (same as above, just with an array as argument
	template<typename Element>
	Plane3(const BasicVector3<Element> points[3]) :
		_normal((points[1] - points[0]).crossProduct(points[2] - points[0]).getNormalised()),
		_dist(points[0].dot(_normal))
	{}

	//	The negation operator for this plane - the normal vector components and the distance are negated
	Plane3 operator- () const
	{
		return Plane3(-_normal, -_dist);
	}

	/**
	 * greebo: Note that planes are considered equal if their normal vectors and
	 * distances don't differ more than an epsilon value.
	 */
	bool operator== (const Plane3& other) const
	{
  		return vector3_equal_epsilon(_normal, other._normal, EPSILON_NORMAL) &&
			   float_equal_epsilon(_dist, other._dist, EPSILON_DIST);
	}

	// Returns the normal vector of this plane
  	Vector3& normal()
	{
    	return _normal;
  	}

	const Vector3& normal() const
	{
		return _normal;
	}

	// Returns the distance of the plane (where the plane intersects the z-axis)
	double& dist()
	{
		return _dist;
	}

	const double& dist() const
	{
		return _dist;
	}

	/* greebo: This normalises the plane by turning the normal vector into a unit vector (dividing it by its length)
	 * and scaling the distance down by the same amount */
	Plane3 getNormalised() const
	{
		double rmagnitudeInv = 1 / _normal.getLength();
  		return Plane3(_normal * rmagnitudeInv, _dist * rmagnitudeInv);
  	}

	// Normalises this Plane3 object in-place
	void normalise()
	{
		double rmagnitudeInv = 1 / _normal.getLength();

		_normal *= rmagnitudeInv;
		_dist *= rmagnitudeInv;
	}

	// Reverses this plane, by negating all components
	void reverse()
	{
		_normal = -_normal;
		_dist = -_dist;
	}

  	Plane3 getTranslated(const Vector3& translation) const
	{
		double distTransformed = -( (-_dist * _normal.x() + translation.x()) * _normal.x() +
									(-_dist * _normal.y() + translation.y()) * _normal.y() +
              						(-_dist * _normal.z() + translation.z()) * _normal.z() );
		return Plane3(_normal, distTransformed);
	}

  	// Checks if the floats of this plane are valid, returns true if this is the case
  	bool isValid() const
	{
		return float_equal_epsilon(_normal.dot(_normal), 1.0, 0.01);
	}

  	/* greebo: Use this to calculate the projection of a <pointToProject> onto this plane.
  	 *
  	 * @returns: the Vector3 pointing to the point on the plane with the shortest
  	 * distance from the passed <pointToProject> */
  	Vector3 getProjection(const Vector3& pointToProject) const
	{
  		// Get the normal vector of this plane and normalise it
  		Vector3 n = _normal.getNormalised();

  		// Retrieve a point of the plane
  		Vector3 planePoint = n*_dist;

  		// Calculate the projection and return it
  		return pointToProject + planePoint - n*pointToProject.dot(n);
  	}

  	/** greebo: Returns the distance to the given point.
  	 */
  	double distanceToPoint(const Vector3& point) const
	{
  		return point.dot(_normal) - _dist;
  	}

  	/* greebo: This calculates the intersection point of three planes.
	 * Returns <0,0,0> if no intersection point could be found, otherwise returns the coordinates of the intersection point
	 * (this may also be 0,0,0) */
	static Vector3 intersect(const Plane3& plane1, const Plane3& plane2, const Plane3& plane3)
	{
		const Vector3& n1 = plane1.normal();
		const Vector3& n2 = plane2.normal();
		const Vector3& n3 = plane3.normal();

		Vector3 n1n2 = n1.crossProduct(n2);
		Vector3 n2n3 = n2.crossProduct(n3);
		Vector3 n3n1 = n3.crossProduct(n1);

		double denom = n1.dot(n2n3);

		// Check if the denominator is zero (which would mean that no intersection is to be found
		if (denom != 0)
		{
			return (n2n3*plane1.dist() + n3n1*plane2.dist() + n1n2*plane3.dist()) / denom;
		}
		else
		{
			// No intersection could be found, return <0,0,0>
			return Vector3(0,0,0);
		}
	}

}; // class Plane3

/**
 * \brief
 * Stream insertion operator for a plane.
 */
inline std::ostream& operator<< (std::ostream& os, const Plane3& plane)
{
    os << "Plane3 { " << plane.normal().x() << "x + "
                      << plane.normal().y() << "y + "
                      << plane.normal().z() << "z = "
                      << plane.dist()
       << " }";
    return os;
}

#endif /*PLANE3_H_*/
