#ifndef ARBITRARYMESHVERTEX_H_
#define ARBITRARYMESHVERTEX_H_

#include "Vertex3f.h"
#include "TexCoord2f.h"

/**
 * Data structure representing a mesh vertex.
 */
struct ArbitraryMeshVertex {
	
	TexCoord2f texcoord;
	Normal3f normal;
	Vertex3f vertex;
	Normal3f tangent;
	Normal3f bitangent;
	
	// Vertex colour
	Vector3 colour;

	/**
	 * Default constructor.
	 */
	ArbitraryMeshVertex() 
	: tangent(0, 0, 0), 
	  bitangent(0, 0, 0),
	  colour(1.0, 1.0, 1.0)
	{ }

	/**
	 * Initialising constructor.
	 */
	ArbitraryMeshVertex(Vertex3f v, Normal3f n, TexCoord2f t)
    : texcoord(t), 
      normal(n), 
      vertex(v), 
      tangent(0, 0, 0), 
      bitangent(0, 0, 0),
      colour(1.0, 1.0, 1.0)
    { }
};

/**
 * String output for ArbitraryMeshVertex.
 */
inline std::ostream& operator<< (std::ostream& os, const ArbitraryMeshVertex& v)
{
	os << "ArbitraryMeshVertex { "
	   << " vertex = " << v.vertex << ", normal = " << v.normal
	   << ", texcoord = " << v.texcoord
	   << " }";
	
	return os;
}

inline bool operator<(const ArbitraryMeshVertex& self, const ArbitraryMeshVertex& other) {
	if(self.texcoord != other.texcoord) {
		return self.texcoord < other.texcoord;
	}
	if(self.normal != other.normal) {
		return self.normal < other.normal;
	}
	if(self.vertex != other.vertex) {
		return self.vertex < other.vertex;
	}
	return false;
}

inline bool operator==(const ArbitraryMeshVertex& self, const ArbitraryMeshVertex& other) {
	return self.texcoord == other.texcoord && self.normal == other.normal && self.vertex == other.vertex;
}

inline bool operator!=(const ArbitraryMeshVertex& self, const ArbitraryMeshVertex& other) {
	return !operator==(self, other);
}

#endif /*ARBITRARYMESHVERTEX_H_*/
