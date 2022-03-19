#pragma once

#include "math/Vector3.h"
#include "render/MeshVertex.h"
#include "render/RenderVertex.h"
#include "math/Hash.h"

/**
* greebo: When creating model surfaces from ASE models, the in-game loading code 
* will aim to merge vertices that are near each other (and have otherwise negligible
* differences for their other components like normal, texcoords and colour).
* 
* The in-game code is using a custom vertex hash index implementation to quickly
* look up almost-similar vertices and prevent such "duplicates" from ending up in the 
* final mesh. This is based on a set of epsilon (one for each type, in CVARs like 
* r_slopVertex, r_slopNormal, etc.) and to speed things up, a hash index is built
* for all the vertices in a single mesh.
* 
* The hash functors below aim to reproduce this behaviour without fully re-implementing
* those custom containers in the engine code, designed to be used in a std::unordered_map
* with MeshVertex used as Key type.
* 
* The MeshVertex hash functions is deliberately coarse and will produce lots of
* collisions for vertices in a certain vicinity, only provide a fast way of looking up 
* duplicates in meshes with many verts. Each "colliding" vertex will then be compared
* in-depth by the std::equal_to<> specialisation, where the epsilon-comparison is performed.
*/
namespace render
{
    // These epsilons below correspond to the CVARs in the game code
    constexpr double VertexEpsilon = 0.01; // r_slopVertex
    constexpr double NormalEpsilon = 0.02; // r_slopNormal
    constexpr double TexCoordEpsilon = 0.001; // r_slopTexCoord
}

// Coarse hash of the 3-component vector
// This is rounding the doubles to the SignificantVertexDigits defined above, 
// so any two vectors with their components only differing after the few significant digits
// will produce the same hash.
template<>
struct std::hash<Vector3>
{
    static constexpr std::size_t SignificantVertexDigits = 2;

    size_t operator()(const Vector3& v) const
    {
        return math::hashVector3(v, SignificantVertexDigits);
    }
};

// Coarse hash of the 3-component float vector
// This is rounding the doubles to the SignificantVertexDigits defined above, 
// so any two vectors with their components only differing after the few significant digits
// will produce the same hash.
template<>
struct std::hash<Vector3f>
{
    static constexpr std::size_t SignificantVertexDigits = 2;

    size_t operator()(const Vector3f& v) const
    {
        return math::hashVector3(v, SignificantVertexDigits);
    }
};

// Hash specialisation such that MeshVertex can be used as key type in std::unordered_map<>
// Only the 3D coordinates (the vertex member) will be considered for hash calculation,
// to intentionally produce hash collisions for vectors that are within a certain VertexEpsilon.
template<>
struct std::hash<MeshVertex>
{
    size_t operator()(const MeshVertex& v) const
    {
        // We just hash the vertex
        return std::hash<Vector3>()(v.vertex);
    }
};

// Assumes equality of two MeshVertices if all of (vertex, normal, texcoord, colour) 
// are equal within defined epsilons (VertexEpsilon, NormalEpsilon, TexCoordEpsilon)
template<>
struct std::equal_to<MeshVertex>
{
    bool operator()(const MeshVertex& a, const MeshVertex& b) const
    {
        return math::isNear(a.vertex, b.vertex, render::VertexEpsilon) &&
            a.normal.dot(b.normal) > (1.0 - render::NormalEpsilon) &&
            math::isNear(a.texcoord, b.texcoord, render::TexCoordEpsilon) &&
            math::isNear(a.colour, b.colour, render::VertexEpsilon);
    }
};

// Hash specialisation such that MeshVertex can be used as key type in std::unordered_map<>
// Only the 3D coordinates (the vertex member) will be considered for hash calculation,
// to intentionally produce hash collisions for vectors that are within a certain VertexEpsilon.
template<>
struct std::hash<render::RenderVertex>
{
    size_t operator()(const render::RenderVertex& v) const
    {
        // We just hash the vertex
        return std::hash<Vector3f>()(v.vertex);
    }
};

// Assumes equality of two MeshVertices if all of (vertex, normal, texcoord, colour) 
// are equal within defined epsilons (VertexEpsilon, NormalEpsilon, TexCoordEpsilon)
template<>
struct std::equal_to<render::RenderVertex>
{
    bool operator()(const render::RenderVertex& a, const render::RenderVertex& b) const
    {
        return math::isNear(a.vertex, b.vertex, render::VertexEpsilon) &&
            a.normal.dot(b.normal) > (1.0 - render::NormalEpsilon) &&
            math::isNear(a.texcoord, b.texcoord, render::TexCoordEpsilon) &&
            math::isNear(a.colour, b.colour, render::VertexEpsilon);
    }
};
