#pragma once

template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;
class Ray;

#include <boost/shared_ptr.hpp>

/**
 * An object supporting this interface provides methods
 * to run traces against its geometry, e.g. calculating 
 * the intersection point of a given Ray. 
 */
class ITraceable
{
public:
	/**
	 * Calculates the intersection point of the given Ray with the geometry
	 * of this object. Returns false if the Ray doesn't have an intersection
	 * point, otherwise returns true, in which case the coordinates of the
	 * intersection point are stored in the intersection reference.
	 */
	virtual bool getIntersection(const Ray& ray, Vector3& intersection) = 0;
};
typedef boost::shared_ptr<ITraceable> ITraceablePtr;
