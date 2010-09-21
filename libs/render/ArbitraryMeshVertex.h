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
	{}

	/**
	 * Initialising constructor.
	 */
	ArbitraryMeshVertex(const Vertex3f& v, const Normal3f& n, const TexCoord2f& t)
    : texcoord(t), 
      normal(n), 
      vertex(v), 
      tangent(0, 0, 0), 
      bitangent(0, 0, 0),
      colour(1.0, 1.0, 1.0)
    {}

	bool operator<(const ArbitraryMeshVertex& other)
	{
		if (texcoord != other.texcoord)
		{
			return texcoord < other.texcoord;
		}

		if (normal != other.normal)
		{
			return normal < other.normal;
		}

		if (vertex != other.vertex)
		{
			return vertex < other.vertex;
		}

		return false;
	}

	bool operator==(const ArbitraryMeshVertex& other)
	{
		return texcoord == other.texcoord && normal == other.normal && vertex == other.vertex;
	}

	bool operator!=(const ArbitraryMeshVertex& other)
	{
		return !operator==(other);
	}
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

#endif /*ARBITRARYMESHVERTEX_H_*/
