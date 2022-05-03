#pragma once

#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"

namespace render
{

/**
 * Vertex as used in DarkRadiant's render system.
 * It's based on single-precision floats for memory/performance.
 */
class RenderVertex
{
public:
    Vector2f texcoord;
    Vector3f normal;
    Vector3f vertex;
    Vector3f tangent;
    Vector3f bitangent;
    Vector4f colour;

    /// Default constructor.
    RenderVertex()
        : tangent(0, 0, 0),
        bitangent(0, 0, 0),
        colour(1.0f, 1.0f, 1.0f, 1.0f)
    {}

    /// Initialising constructor, leaves tangent vectors at 0,0,0
    RenderVertex(const Vector3f& v, const Vector3f& n, const Vector2f& t,
                 const Vector4f& c = {1.0f, 1.0f, 1.0f, 1.0f})
    : RenderVertex(v, n, t, c, {0, 0, 0}, {0, 0, 0})
    {
    }

    /// Construct and initialise all values
    template <typename U>
    RenderVertex(const BasicVector3<U>& vertex_, const BasicVector3<U>& normal_,
                 const BasicVector2<U>& texcoord_, const BasicVector4<U>& colour_,
                 const BasicVector3<U>& tangent_ = BasicVector3<U>(0, 0, 0),
                 const BasicVector3<U>& bitangent_ = BasicVector3<U>(0, 0, 0))
    : texcoord(texcoord_),
      normal(normal_),
      vertex(vertex_),
      tangent(tangent_),
      bitangent(bitangent_),
      colour(colour_)
    {}
};

}
