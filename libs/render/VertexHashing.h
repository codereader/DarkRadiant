#pragma once

#include "math/Vector3.h"
#include "render/ArbitraryMeshVertex.h"

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
* with ArbitraryMeshVertex used as Key type.
* 
* The ArbitraryMeshVertex hash functions is deliberately coarse and will produce lots of
* collisions for vertices in a certain vicinity, only provide a fast way of looking up 
* duplicates in meshes with many verts. Each "colliding" vertex will then be compared
* in-depth by the std::equal_to<> specialisation, where the epsilon-comparison is performed.
*/
namespace render
{
    namespace detail
    {
        // A hash combination function based on the one used in boost and found on stackoverflow
        inline void combineHash(std::size_t& seed, std::size_t hash)
        {
            seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        // Delivers 10.0^signficantDigits
        constexpr double RoundingFactor(std::size_t significantVertexDigits)
        {
            return significantVertexDigits == 1 ? 10.0 : 10.0 * RoundingFactor(significantVertexDigits - 1);
        }
    }

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
        auto xHash = static_cast<std::size_t>(v.x() * render::detail::RoundingFactor(SignificantVertexDigits));
        auto yHash = static_cast<std::size_t>(v.y() * render::detail::RoundingFactor(SignificantVertexDigits));
        auto zHash = static_cast<std::size_t>(v.z() * render::detail::RoundingFactor(SignificantVertexDigits));

        render::detail::combineHash(xHash, yHash);
        render::detail::combineHash(xHash, zHash);

        return xHash;
    }
};

// Hash specialisation such that ArbitraryMeshVertex can be used as key type in std::unordered_map<>
// Only the 3D coordinates (the vertex member) will be considered for hash calculation,
// to intentionally produce hash collisions for vectors that are within a certain VertexEpsilon.
template<>
struct std::hash<ArbitraryMeshVertex>
{
    size_t operator()(const ArbitraryMeshVertex& v) const
    {
        // We just hash the vertex
        return std::hash<Vector3>()(v.vertex);
    }
};

// Assumes equality of two ArbitraryMeshVertices if all of (vertex, normal, texcoord, colour) 
// are equal within defined epsilons (VertexEpsilon, NormalEpsilon, TexCoordEpsilon)
template<>
struct std::equal_to<ArbitraryMeshVertex>
{
    bool operator()(const ArbitraryMeshVertex& a, const ArbitraryMeshVertex& b) const
    {
        return math::isNear(a.vertex, b.vertex, render::VertexEpsilon) &&
            a.normal.dot(b.normal) > (1.0 - render::NormalEpsilon) &&
            math::isNear(a.texcoord, b.texcoord, render::TexCoordEpsilon) &&
            math::isNear(a.colour, b.colour, render::VertexEpsilon);
    }
};
