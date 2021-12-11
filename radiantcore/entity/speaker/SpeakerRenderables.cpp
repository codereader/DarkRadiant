#include "SpeakerRenderables.h"

#include "igl.h"
#include "render.h"

void sphereDrawFill(const Vector3& origin, float radius, int sides)
{
  if (radius <= 0)
    return;

  const double dt = c_2pi / static_cast<float>(sides);
  const double dp = math::PI / static_cast<float>(sides);

  glBegin(GL_TRIANGLES);
  for (int i = 0; i <= sides - 1; ++i)
  {
    for (int j = 0; j <= sides - 2; ++j)
    {
      const double t = i * dt;
      const double p = (j * dp) - (math::PI / 2.0);

      {
        Vector3 v(origin + Vector3::createForSpherical(t, p) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + Vector3::createForSpherical(t, p + dp) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + Vector3::createForSpherical(t + dt, p + dp) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + Vector3::createForSpherical(t, p) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + Vector3::createForSpherical(t + dt, p + dp) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + Vector3::createForSpherical(t + dt, p) * radius);
        glVertex3dv(v);
      }
    }
  }

  {
    const double p = (sides - 1) * dp - (static_cast<float>(math::PI) / 2.0f);
    for (int i = 0; i <= sides - 1; ++i)
    {
      const double t = i * dt;

      {
        Vector3 v(origin + Vector3::createForSpherical(t, p) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + Vector3::createForSpherical(t + dt, p + dp) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + Vector3::createForSpherical(t + dt, p) * radius);
        glVertex3dv(v);
      }
    }
  }
  glEnd();
}

void speakerDrawRadiiFill(const Vector3& origin, const SoundRadii& rad)
{
  if(rad.getMin() > 0)
    sphereDrawFill(origin, rad.getMin(), 16);
  if(rad.getMax() > 0)
    sphereDrawFill(origin, rad.getMax(), 16);
}

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

} // namespace
