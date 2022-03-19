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

    /// Initialising constructor, leaves colour at 1,1,1,1 and tangent vectors at 0,0,0
    RenderVertex(const Vector3f& v, const Vector3f& n, const Vector2f& t) :
        RenderVertex(v, n, t, { 1.0f, 1.0f, 1.0f, 1.0f })
    {}

    /// Initialising constructor, leaves tangent vectors at 0,0,0
    RenderVertex(const Vector3f& v, const Vector3f& n, const Vector2f& t, const Vector4f& c) :
        RenderVertex(v, n, t, c, { 0, 0, 0 }, { 0, 0, 0 })
    {}

    // Initialises all attributes of this vertex
    RenderVertex(const Vector3f& vertex_, const Vector3f& normal_,
        const Vector2f& texcoord_, const Vector4f& colour_,
        const Vector3f& tangent_, const Vector3f& bitangent_) :
        texcoord(texcoord_),
        normal(normal_),
        vertex(vertex_),
        tangent(tangent_),
        bitangent(bitangent_),
        colour(colour_)
    {}

    // Adapter constructor converting the incoming double-precision vectors
    // into single-precision floats. Variant without tangent/bitangent.
    RenderVertex(const Vector3& vertex_, const Vector3& normal_,
        const Vector2& texcoord_, const Vector4& colour_) :
        RenderVertex(vertex_, normal_, texcoord_, colour_, { 0, 0, 0 }, { 0, 0, 0 })
    {}

    // Adapter constructor converting the incoming double-precision vectors
    // into single-precision floats
    RenderVertex(const Vector3& vertex_, const Vector3& normal_,
        const Vector2& texcoord_, const Vector4& colour_,
        const Vector3& tangent_, const Vector3& bitangent_) :
        texcoord(static_cast<float>(texcoord_.x()), static_cast<float>(texcoord_.y())),
        normal(static_cast<float>(normal_.x()), static_cast<float>(normal_.y()), static_cast<float>(normal_.z())),
        vertex(static_cast<float>(vertex_.x()), static_cast<float>(vertex_.y()), static_cast<float>(vertex_.z())),
        tangent(static_cast<float>(tangent_.x()), static_cast<float>(tangent_.y()), static_cast<float>(tangent_.z())),
        bitangent(static_cast<float>(bitangent_.x()), static_cast<float>(bitangent_.y()), static_cast<float>(bitangent_.z())),
        colour(static_cast<float>(colour_.x()), static_cast<float>(colour_.y()), static_cast<float>(colour_.z()), static_cast<float>(colour_.w()))
    {}
};

}
