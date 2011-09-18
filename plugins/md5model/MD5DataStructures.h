#pragma once

#include <vector>
#include "math/Vector3.h"
#include "math/Quaternion.h"

/** greebo: Some data structures used in MD5 model code
 */
namespace md5
{

/**
 * Data structure containing MD5 Joint information.
 */
struct MD5Joint
{
	int parent;
	Vector3 position;
	Quaternion rotation;
};

typedef std::vector<MD5Joint> MD5Joints;

#ifdef _DEBUG

// Stream insertion for MD5Joint
inline std::ostream& operator<< (std::ostream& os, const MD5Joint& jt) {
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
	std::size_t index;	// Weight index
	std::size_t joint;	// Joint ID
	float t;			// Weight
	Vector3 v;			// Vertex Posistion relative to the Bone
};

typedef std::vector<MD5Weight> MD5Weights;

// The combination of vertices, triangles and weighting information
// represents our MD5 mesh - using this info it's possible to create
// the actual rendered geometry (position, normals, etc.)
struct MD5Mesh
{
	MD5Verts	vertices;
	MD5Tris		triangles;
	MD5Weights	weights;
};
typedef boost::shared_ptr<MD5Mesh> MD5MeshPtr;

} // namespace
