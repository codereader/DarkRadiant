#include "SpeakerRenderables.h"

#include "ientity.h"
#include "render.h"
#include "entity/EntityNode.h"

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

    std::vector<render::RenderVertex> vertices;

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

    auto colour = _entity.getRenderState() == scene::INode::RenderState::Active ?
        _entity.getEntityColour() : INACTIVE_ENTITY_COLOUR;

    // Move the points to their world position
    for (auto& vertex : vertices)
    {
        vertex.vertex.x() += static_cast<float>(_origin.x());
        vertex.vertex.y() += static_cast<float>(_origin.y());
        vertex.vertex.z() += static_cast<float>(_origin.z());

        vertex.colour = Vector4f{ static_cast<float>(colour.x()), static_cast<float>(colour.y()),
            static_cast<float>(colour.z()), static_cast<float>(colour.w()) };
    }

    updateGeometryWithData(render::GeometryType::Lines, vertices, CircleIndices);
}

// ---- Fill Variant ----

namespace
{

constexpr std::size_t ThetaDivisions = 8; // Inclination divisions
constexpr std::size_t PhiDivisions = 16;  // Azimuth divisions

const double ThetaStep = math::PI / ThetaDivisions;
const double PhiStep = 2 * math::PI / PhiDivisions;

// For one sphere, we have (divisions-1) vertex circles between the top and bottom vertex
constexpr auto NumCircles = ThetaDivisions - 1;

constexpr auto NumVerticesPerSphere = (NumCircles * PhiDivisions + 2);
constexpr auto NumVerticesPerCircle = static_cast<unsigned int>(NumVerticesPerSphere - 2) / NumCircles;

inline void generateSphereIndices(std::vector<unsigned int>& indices, unsigned int sphereOffset)
{
    for (unsigned int circle = 0; circle < NumCircles - 1; ++circle)
    {
        unsigned int offset = sphereOffset + circle * NumVerticesPerCircle;

        for (unsigned int i = 0; i < NumVerticesPerCircle; ++i)
        {
            indices.push_back(offset + i);
            indices.push_back(offset + (i + 1) % NumVerticesPerCircle); // wrap
            indices.push_back(offset + NumVerticesPerCircle + (i + 1) % NumVerticesPerCircle); // wrap
            indices.push_back(offset + NumVerticesPerCircle + i);
        }
    }

    // Connect the topmost circle to the top pole vertex
    // These are tris, but we cannot mix, so let's form a degenerate quad
    auto topPoleIndex = sphereOffset + static_cast<unsigned int>(NumVerticesPerSphere) - 2;
    auto bottomPoleIndex = sphereOffset + static_cast<unsigned int>(NumVerticesPerSphere) - 1;

    for (unsigned int i = 0; i < NumVerticesPerCircle; ++i)
    {
        indices.push_back(topPoleIndex);
        indices.push_back(topPoleIndex);
        indices.push_back(sphereOffset + (i + 1) % NumVerticesPerCircle); // wrap
        indices.push_back(sphereOffset + i);
    }

    // Connect the most southern circle to the south pole vertex
    auto bottomCircleOffset = sphereOffset + static_cast<unsigned int>((NumCircles - 1) * NumVerticesPerCircle);

    for (unsigned int i = 0; i < NumVerticesPerCircle; ++i)
    {
        indices.push_back(bottomCircleOffset + i);
        indices.push_back(bottomCircleOffset + (i + 1) % NumVerticesPerCircle); // wrap
        indices.push_back(bottomPoleIndex);
        indices.push_back(bottomPoleIndex);
    }
}

// Generates the draw indices for a sphere with N circles plus two pole vertices at the back
inline std::vector<unsigned int> generateSphereIndices()
{
    std::vector<unsigned int> indices;

    indices.reserve((NumCircles + 1) * NumVerticesPerCircle * 4 * 2); // 4 indices per quad, 2 spheres

    // Generate the two index sets, one for each sphere
    generateSphereIndices(indices, 0);
    generateSphereIndices(indices, NumVerticesPerSphere);

    assert((NumCircles + 1) * NumVerticesPerCircle * 4 * 2 == indices.size());

    return indices;
}

}

void RenderableSpeakerRadiiFill::generateSphereVertices(std::vector<render::RenderVertex>& vertices, double radius)
{
    auto colour = _entity.getRenderState() == scene::INode::RenderState::Active ?
        _entity.getEntityColour() : INACTIVE_ENTITY_COLOUR;
    colour.w() = 0.3;

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
            vertices.push_back(render::RenderVertex(unit * radius + _origin, unit, { 0, 0 }, colour));
        }
    }

    // The north and south pole vertices
    vertices.push_back(render::RenderVertex(Vector3(0, 0, radius) + _origin, { 0,0,1 }, { 0,0 }, colour));
    vertices.push_back(render::RenderVertex(Vector3(0, 0, -radius) + _origin, { 0,0,-1 }, { 0,0 }, colour));
}

void RenderableSpeakerRadiiFill::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;

    std::vector<render::RenderVertex> vertices;

    // Make space for two spheres
    vertices.reserve(NumVerticesPerSphere << 1);

    generateSphereVertices(vertices, _radii.getMax());
    generateSphereVertices(vertices, _radii.getMin());

    // Generate the quad indices for two spheres
    static auto SphereIndices = generateSphereIndices();

    updateGeometryWithData(render::GeometryType::Quads, vertices, SphereIndices);
}

} // namespace
