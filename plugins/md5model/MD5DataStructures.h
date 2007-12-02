#ifndef MD5DATASTRUCTURES_H_
#define MD5DATASTRUCTURES_H_

#include <vector>
#include "math/Vector3.h"

/** greebo: These are some data structures used by the MD5Parser
 */

namespace md5 {

/**
 * Data structure containing MD5 Joint information.
 */
struct MD5Joint
{
	int parent;
	Vector3 position;
	Vector4 rotation;
};

typedef std::vector<MD5Joint> MD5Joints;

#ifdef _DEBUG

// Stream insertion for MD5Joint
std::ostream& operator<< (std::ostream& os, const MD5Joint& jt) {
	os << "MD5Joint { parent=" << jt.parent
	   << " position=" << jt.position
	   << " rotation=" << jt.rotation
	   << " }";
	return os;
}

#endif

/**
 * Data structure containing MD5 Vertex information. Vertices do not contain 
 * their own positional information, but are instead attached to joints
 * according to one or more "weight" parameters.
 */
struct MD5Vert
{
	std::size_t index;
	float u;
	float v;
	std::size_t weight_index;
	std::size_t weight_count;
};

typedef std::vector<MD5Vert> MD5Verts;

/**
 * Data structure containing MD5 triangle information. A triangle connects
 * three vertices, addressed by index.
 */
struct MD5Tri
{
	std::size_t index;
	std::size_t a;
	std::size_t b;
	std::size_t c;
};

typedef std::vector<MD5Tri> MD5Tris;

/**
 * Data structure containing weight information.
 */
struct MD5Weight
{
	std::size_t index;
	std::size_t joint;
	float t;
	Vector3 v;
};

typedef std::vector<MD5Weight> MD5Weights;

typedef float MD5Component;
typedef std::vector<MD5Component> MD5Components;

class MD5Frame
{
public:
	MD5Components m_components;
};

} // namespace md5

#endif /*MD5DATASTRUCTURES_H_*/
