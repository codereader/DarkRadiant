#pragma once

#include <cstddef>

#include "Vertex3f.h"
#include "TexCoord2f.h"
#include "VertexTraits.h"

/**
 * Data structure representing a mesh vertex.
 */
class ArbitraryMeshVertex
{
public:
	TexCoord2f	texcoord;
	Normal3f	normal;
	Vertex3f	vertex;
	Normal3f	tangent;
	Normal3f	bitangent;

	// Vertex colour
	Vector3		colour;

	/// Default constructor.
	ArbitraryMeshVertex()
	: tangent(0, 0, 0),
	  bitangent(0, 0, 0),
	  colour(1.0, 1.0, 1.0)
	{}

	/// Initialising constructor.
	ArbitraryMeshVertex(const Vertex3f& v, const Normal3f& n, const TexCoord2f& t)
    : texcoord(t),
      normal(n),
      vertex(v),
      tangent(0, 0, 0),
      bitangent(0, 0, 0),
      colour(1.0, 1.0, 1.0)
    {}

    /// Cast to simple Vertex3f, throwing away other components
    operator Vertex3f() const
    {
        return vertex;
    }
};

/// Less-than comparison for ArbitraryMeshVertex
inline bool operator<(const ArbitraryMeshVertex& first,
                      const ArbitraryMeshVertex& other)
{
    if (first.texcoord != other.texcoord)
    {
        return first.texcoord < other.texcoord;
    }

    if (first.normal != other.normal)
    {
        return first.normal < other.normal;
    }

    if (first.vertex != other.vertex)
    {
        return first.vertex < other.vertex;
    }

    return false;
}

/// Equality comparison for ArbitraryMeshVertex
inline bool operator==(const ArbitraryMeshVertex& first,
                       const ArbitraryMeshVertex& other)
{
    return first.texcoord == other.texcoord
        && first.normal == other.normal
        && first.vertex == other.vertex;
}

/// Inequality comparison for ArbitraryMeshVertex
inline bool operator!=(const ArbitraryMeshVertex& first,
                       const ArbitraryMeshVertex& other)
{
    return !(first == other);
}

namespace render
{

/// VertexTraits specialisation for ArbitraryMeshVertex
template<> class VertexTraits<ArbitraryMeshVertex>
{
public:
    static const void* VERTEX_OFFSET()
    {
        return reinterpret_cast<const void*>(
            offsetof(ArbitraryMeshVertex, vertex)
        );
    }

    static bool hasNormal() { return true; }
    static const void* NORMAL_OFFSET()
    {
        return reinterpret_cast<const void*>(
            offsetof(ArbitraryMeshVertex, normal)
        );
    }

    static bool hasTexCoord() { return true; }
    static const void* TEXCOORD_OFFSET()
    {
        return reinterpret_cast<const void*>(
            offsetof(ArbitraryMeshVertex, texcoord)
        );
    }

    static bool hasTangents() { return true; }
    static const void* TANGENT_OFFSET()
    {
        return reinterpret_cast<const void*>(
            offsetof(ArbitraryMeshVertex, tangent)
        );
    }
    static const void* BITANGENT_OFFSET()
    {
        return reinterpret_cast<const void*>(
            offsetof(ArbitraryMeshVertex, bitangent)
        );
    }
};

}

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

/// \brief Calculates the tangent vectors for a triangle \p a, \p b, \p c and stores the tangent in \p s and the bitangent in \p t.
inline void ArbitraryMeshTriangle_calcTangents(const ArbitraryMeshVertex& a,
   const ArbitraryMeshVertex& b, const ArbitraryMeshVertex& c,
   Vector3& s, Vector3& t)
{
	s = Vector3(0, 0, 0);
	t = Vector3(0, 0, 0);
	Vector3 aVec, bVec, cVec;

	{
		aVec.set(a.vertex.x(), a.texcoord.s(), a.texcoord.t());
		bVec.set(b.vertex.x(), b.texcoord.s(), b.texcoord.t());
		cVec.set(c.vertex.x(), c.texcoord.s(), c.texcoord.t());

		Vector3 cross( (bVec-aVec).crossProduct(cVec-aVec) );

		if(fabs(cross.x()) > 0.000001f) {
			s.x() = -cross.y() / cross.x();
		}

		if(fabs(cross.x()) > 0.000001f) {
			t.x() = -cross.z() / cross.x();
		}
	}

	{
		aVec.set(a.vertex.y(), a.texcoord.s(), a.texcoord.t());
		bVec.set(b.vertex.y(), b.texcoord.s(), b.texcoord.t());
		cVec.set(c.vertex.y(), c.texcoord.s(), c.texcoord.t());

		Vector3 cross( (bVec-aVec).crossProduct(cVec-aVec));

		if(fabs(cross.x()) > 0.000001f) {
			s.y() = -cross.y() / cross.x();
		}

		if(fabs(cross.x()) > 0.000001f) {
			t.y() = -cross.z() / cross.x();
		}
	}

	{
		aVec.set(a.vertex.z(), a.texcoord.s(), a.texcoord.t());
		bVec.set(b.vertex.z(), b.texcoord.s(), b.texcoord.t());
		cVec.set(c.vertex.z(), c.texcoord.s(), c.texcoord.t());

		Vector3 cross( (bVec-aVec).crossProduct(cVec-aVec));

		if(fabs(cross.x()) > 0.000001f) {
			s.z() = -cross.y() / cross.x();
		}

		if(fabs(cross.x()) > 0.000001f) {
			t.z() = -cross.z() / cross.x();
		}
	}
}

inline void ArbitraryMeshTriangle_sumTangents(ArbitraryMeshVertex& a, ArbitraryMeshVertex& b, ArbitraryMeshVertex& c)
{
	Vector3 s, t;

	ArbitraryMeshTriangle_calcTangents(a, b, c, s, t);

	a.tangent += s;
	b.tangent += s;
	c.tangent += s;

	a.bitangent += t;
	b.bitangent += t;
	c.bitangent += t;
}
