#include "SpeakerRenderables.h"

#include "render.h"

namespace entity
{

// Generates the draw indices for N circles stored in the given vertex array
inline std::vector<unsigned int> generateWireframeCircleIndices(std::size_t numVertices, unsigned int numCircles)
{
    std::vector<unsigned int> indices;

    indices.reserve(numVertices << 1); // 2 indices per vertex
    const auto numVerticesPerCircle = static_cast<unsigned int>(numVertices) / numCircles;

    for (unsigned int circle = 0; circle < numCircles; ++circle)
    {
        unsigned int offset = circle * numVerticesPerCircle;

        for (unsigned int i = 0; i < numVerticesPerCircle; ++i)
        {
            indices.push_back(offset + i);
            indices.push_back(offset + (i + 1) % numVerticesPerCircle); // wrap around the last index to point at <offset> again
        }
    }

    return indices;
}

// ---- Wireframe Variant ----

void RenderableSpeakerRadiiWireframe::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;

    // Generate the three circles in axis-aligned planes
    constexpr std::size_t NumSegments = 2;

    std::vector<ArbitraryMeshVertex> vertices;

    // Allocate space for 6 circles, one per radius, each radius has NumSegments * 8 vertices
    constexpr std::size_t NumVerticesPerCircle = NumSegments << 3;
    constexpr unsigned int NumCircles = 6;

    vertices.resize(NumCircles * NumVerticesPerCircle);

    // Min radius
    draw_circle<RemapXYZ>(NumSegments, _radii.getMin(), vertices, 0);
    draw_circle<RemapYZX>(NumSegments, _radii.getMin(), vertices, NumVerticesPerCircle);
    draw_circle<RemapZXY>(NumSegments, _radii.getMin(), vertices, NumVerticesPerCircle * 2);

    // Max radius
    draw_circle<RemapXYZ>(NumSegments, _radii.getMax(), vertices, NumVerticesPerCircle * 3);
    draw_circle<RemapYZX>(NumSegments, _radii.getMax(), vertices, NumVerticesPerCircle * 4);
    draw_circle<RemapZXY>(NumSegments, _radii.getMax(), vertices, NumVerticesPerCircle * 5);

    // Generate the indices for all 6 circles, walking around each circle
    // The indices for 6 circles stay the same, so we can store this statically
    static auto CircleIndices = generateWireframeCircleIndices(vertices.size(), NumCircles);

    // Move the points to their world position
    for (auto& vertex : vertices)
    {
        vertex.vertex += _origin;
    }

    RenderableGeometry::updateGeometry(render::GeometryType::Lines, vertices, CircleIndices);
}

// ---- Fill Variant ----

// Generates the draw indices for a sphere with N circles plus two pole vertices at the back
inline std::vector<unsigned int> generateSphereIndices(std::size_t numVertices, unsigned int numCircles)
{
    assert(numVertices > 2);

    std::vector<unsigned int> indices;

    const auto numVerticesPerCircle = static_cast<unsigned int>(numVertices - 2) / numCircles;

    indices.reserve((numCircles + 1) * numVerticesPerCircle * 4); // 4 indices per quad

    for (unsigned int circle = 0; circle < numCircles - 1; ++circle)
    {
        unsigned int offset = circle * numVerticesPerCircle;

        for (unsigned int i = 0; i < numVerticesPerCircle; ++i)
        {
            indices.push_back(offset + i);
            indices.push_back(offset + (i + 1) % numVerticesPerCircle); // wrap
            indices.push_back(offset + numVerticesPerCircle + (i + 1) % numVerticesPerCircle); // wrap
            indices.push_back(offset + numVerticesPerCircle + i);
        }
    }

    // Connect the topmost circle to the top pole vertex
    // These are tris, but we cannot mix, so let's form a degenerate quad
    auto topPoleIndex = static_cast<unsigned int>(numVertices) - 2;
    auto bottomPoleIndex = static_cast<unsigned int>(numVertices) - 1;
    
    for (unsigned int i = 0; i < numVerticesPerCircle; ++i)
    {
        indices.push_back(topPoleIndex);
        indices.push_back(topPoleIndex);
        indices.push_back((i + 1) % numVerticesPerCircle); // wrap
        indices.push_back(i);
    }

    // Connect the most southern circle to the south pole vertex
    auto bottomCircleOffset = (numCircles - 1) * numVerticesPerCircle;

    for (unsigned int i = 0; i < numVerticesPerCircle; ++i)
    {
        indices.push_back(bottomCircleOffset + i);
        indices.push_back(bottomCircleOffset + (i + 1) % numVerticesPerCircle); // wrap
        indices.push_back(bottomPoleIndex);
        indices.push_back(bottomPoleIndex);
    }

    assert((numCircles + 1) * numVerticesPerCircle * 4 == indices.size());

    return indices;
}

void RenderableSpeakerRadiiFill::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;

    constexpr std::size_t ThetaDivisions = 8; // Inclination divisions
    constexpr std::size_t PhiDivisions = 16;  // Azimuth divisions

    const double ThetaStep = math::PI / ThetaDivisions;
    const double PhiStep = 2 * math::PI / PhiDivisions;

    // We have (divisions-1) vertex circles between the top and bottom vertex
    constexpr auto NumCircles = ThetaDivisions - 1;

    std::vector<ArbitraryMeshVertex> vertices;

    // Reserve the vertices for the two pole vertices and the circles
    vertices.reserve(NumCircles * PhiDivisions + 2);

    auto radius = _radii.getMax();

    for (auto strip = 0; strip < NumCircles; ++strip)
    {
        auto theta = ThetaStep * (strip + 1);
        auto z = cos(theta);

        for (auto p = 0; p < PhiDivisions; ++p)
        {
            auto phi = PhiStep * p;
            auto sinTheta = sin(theta);
            auto cosTheta = cos(theta);

            // We use the unit direction for the normal at that vertex
            auto unit = Vector3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

            // Move the points to their world position
            vertices.push_back(ArbitraryMeshVertex(unit * radius + _origin, unit, {0, 0}));
        }
    }

    // The north and south pole vertices
    vertices.push_back(ArbitraryMeshVertex(Vector3(0, 0, radius) + _origin, { 0,0,1 }, { 0,0 }));
    vertices.push_back(ArbitraryMeshVertex(Vector3(0, 0, -radius) + _origin, { 0,0,-1 }, { 0,0 }));

    assert(vertices.size() == NumCircles * PhiDivisions + 2);

    // Generate the triangle indices
    static auto SphereIndices = generateSphereIndices(vertices.size(), NumCircles);

    RenderableGeometry::updateGeometry(render::GeometryType::Quads, vertices, SphereIndices);
}

} // namespace
