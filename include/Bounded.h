#ifndef BOUNDED_H_
#define BOUNDED_H_

#include <boost/shared_ptr.hpp>

/* FOWARD DECLS */
class AABB;

/**
 * Interface for bounded objects, which maintain a local AABB.
 */
class Bounded
{
public:
	/**
	 * Return the local AABB for this object.
	 */
	virtual const AABB& localAABB() const = 0;
};
typedef boost::shared_ptr<Bounded> BoundedPtr;

#endif /*BOUNDED_H_*/
