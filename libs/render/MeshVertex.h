#pragma once

#include <cstddef>

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "Vertex3f.h"
#include "TexCoord2f.h"
#include "VertexTraits.h"

/**
 * Data structure representing a mesh vertex.
 */
class MeshVertex
{
public:
	TexCoord2f	texcoord;
	Normal3	normal;
	Vertex3	vertex;
	Normal3	tangent;
	Normal3	bitangent;

	// Vertex colour
	Vector4		colour;

	/// Default constructor.
	MeshVertex()
	: tangent(0, 0, 0),
	  bitangent(0, 0, 0),
	  colour(1.0, 1.0, 1.0, 1.0)
	{}

	/// Initialising constructor, leaves colour at 1,1,1,1 and tangent vectors at 0,0,0
	MeshVertex(const Vertex3& v, const Normal3& n, const TexCoord2f& t) : 
        MeshVertex(v, n, t, { 1.0, 1.0, 1.0, 1.0 })
    {}

	/// Initialising constructor, leaves tangent vectors at 0,0,0
    MeshVertex(const Vertex3& v, const Normal3& n, const TexCoord2f& t, const Vector4& c) : 
        MeshVertex(v, n, t, c, { 0, 0, 0 }, { 0, 0, 0 })
    {}

    // Initialises all attributes of this vertex
    MeshVertex(const Vertex3& vertex_, const Normal3& normal_, 
               const TexCoord2f& texcoord_, const Vector4& colour_, 
               const Normal3& tangent_, const Normal3& bitangent_) :
        texcoord(texcoord_),
        normal(normal_),
        vertex(vertex_),
        tangent(tangent_),
        bitangent(bitangent_),
        colour(colour_)
    {}

    /// Cast to simple Vertex3, throwing away other components
    operator Vertex3() const
    {
        return vertex;
    }
};

/// Less-than comparison for MeshVertex
inline bool operator<(const MeshVertex& first,
                      const MeshVertex& other)
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

/// Equality comparison for MeshVertex
inline bool operator==(const MeshVertex& first,
                       const MeshVertex& other)
{
    return first.texcoord == other.texcoord
        && first.normal == other.normal
        && first.vertex == other.vertex;
}

/// Inequality comparison for MeshVertex
inline bool operator!=(const MeshVertex& first,
                       const MeshVertex& other)
{
    return !(first == other);
}

namespace render
{

/// VertexTraits specialisation for MeshVertex
template<> class VertexTraits<MeshVertex>
{
public:
    static const void* VERTEX_OFFSET()
    {
        return reinterpret_cast<const void*>(
            offsetof(MeshVertex, vertex)
        );
    }

    static bool hasNormal() { return true; }
    static const void* NORMAL_OFFSET()
    {
        return reinterpret_cast<const void*>(
            offsetof(MeshVertex, normal)
        );
    }

    static bool hasTexCoord() { return true; }
    static const void* TEXCOORD_OFFSET()
    {
        return reinterpret_cast<const void*>(
            offsetof(MeshVertex, texcoord)
        );
    }

    static bool hasTangents() { return true; }
    static const void* TANGENT_OFFSET()
    {
        return reinterpret_cast<const void*>(
            offsetof(MeshVertex, tangent)
        );
    }
    static const void* BITANGENT_OFFSET()
    {
        return reinterpret_cast<const void*>(
            offsetof(MeshVertex, bitangent)
        );
    }
};

}

/**
 * String output for MeshVertex.
 */
inline std::ostream& operator<< (std::ostream& os, const MeshVertex& v)
{
	os << "MeshVertex { "
	   << " vertex = " << v.vertex << ", normal = " << v.normal
	   << ", texcoord = " << v.texcoord << ", colour = " << v.colour
	   << " }";

	return os;
}

/// \brief Calculates the tangent vectors for a triangle \p a, \p b, \p c and stores the tangent in \p s and the bitangent in \p t.
inline void MeshTriangle_calcTangents(const MeshVertex& a,
   const MeshVertex& b, const MeshVertex& c,
   Vector3& s, Vector3& t)
{
	s = Vector3(0, 0, 0);
	t = Vector3(0, 0, 0);
	Vector3 aVec, bVec, cVec;

	{
		aVec.set(a.vertex.x(), a.texcoord.s(), a.texcoord.t());
		bVec.set(b.vertex.x(), b.texcoord.s(), b.texcoord.t());
		cVec.set(c.vertex.x(), c.texcoord.s(), c.texcoord.t());

		Vector3 cross( (bVec-aVec).cross(cVec-aVec) );

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

		Vector3 cross( (bVec-aVec).cross(cVec-aVec));

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

		Vector3 cross( (bVec-aVec).cross(cVec-aVec));

		if(fabs(cross.x()) > 0.000001f) {
			s.z() = -cross.y() / cross.x();
		}

		if(fabs(cross.x()) > 0.000001f) {
			t.z() = -cross.z() / cross.x();
		}
	}
}

inline void MeshTriangle_sumTangents(MeshVertex& a, MeshVertex& b, MeshVertex& c)
{
	Vector3 s, t;

	MeshTriangle_calcTangents(a, b, c, s, t);

	a.tangent += s;
	b.tangent += s;
	c.tangent += s;

	a.bitangent += t;
	b.bitangent += t;
	c.bitangent += t;
}
