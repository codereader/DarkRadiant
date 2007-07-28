#ifndef VERTEX3F_H_
#define VERTEX3F_H_

#include "math/Vector3.h"

/// \brief A 3-float vertex.
class Vertex3f : 
	public Vector3
{
public:
	/** Default constructor.
	 */
	Vertex3f()
	{}

	/** Construct a Vertex3f from 3 individual values
	 */
	Vertex3f(double _x, double _y, double _z) : 
		Vector3(_x, _y, _z)
	{}

	/** Construct a Vertex3f from a 3-element array
	 */
	Vertex3f(const double* array) : 
		Vector3(array)
	{}
	
	// static Named constructor 
	static Vertex3f Identity() {
		return Vertex3f(0,0,0);
	}

	bool operator<(const Vertex3f& other) const {
		if (x() != other.x()) {
			return x() < other.x();
		}
		if (y() != other.y()) {
			return y() < other.y();
		}
		if (z() != other.z()) {
			return z() < other.z();
		}
		return false;
	}
};

/* Normal3f typedef */
typedef Vertex3f Normal3f;

#endif /*VERTEX3F_H_*/
