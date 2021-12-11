#include "SpeakerRenderables.h"

#include "render.h"

// the drawing functions

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

void sphereDrawWire(const Vector3& origin, float radius, int sides)
{
  {
    glBegin(GL_LINE_LOOP);

    for (int i = 0; i <= sides; i++)
    {
      float ds = sin((i * 2 * static_cast<float>(math::PI)) / sides);
      float dc = cos((i * 2 * static_cast<float>(math::PI)) / sides);

      glVertex3d(
        origin[0] + radius * dc,
        origin[1] + radius * ds,
        origin[2]
      );
    }

    glEnd();
  }

  {
    glBegin(GL_LINE_LOOP);

    for (int i = 0; i <= sides; i++)
    {
      float ds = sin((i * 2 * static_cast<float>(math::PI)) / sides);
      float dc = cos((i * 2 * static_cast<float>(math::PI)) / sides);

      glVertex3d(
        origin[0] + radius * dc,
        origin[1],
        origin[2] + radius * ds
      );
    }

    glEnd();
  }

  {
    glBegin(GL_LINE_LOOP);

    for (int i = 0; i <= sides; i++)
    {
      float ds = sin((i * 2 * static_cast<float>(math::PI)) / sides);
      float dc = cos((i * 2 * static_cast<float>(math::PI)) / sides);

      glVertex3d(
        origin[0],
        origin[1] + radius * dc,
        origin[2] + radius * ds
      );
    }

    glEnd();
  }
}

void speakerDrawRadiiWire(const Vector3& origin, const SoundRadii& rad)
{
  if(rad.getMin() > 0)
    sphereDrawWire(origin, rad.getMin(), 24);
  if(rad.getMax() > 0)
    sphereDrawWire(origin, rad.getMax(), 24);
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

#if 0
void RenderableSpeakerRadii::render(const RenderInfo& info) const
{
	//draw the radii of speaker based on speaker shader/radii keys
	if(info.checkFlag(RENDER_FILL)) {
		speakerDrawRadiiFill(Vector3(0,0,0), m_radii);
	}
	else {
		speakerDrawRadiiWire(Vector3(0,0,0), m_radii);
	}
}
#endif

#if 0
const AABB& RenderableSpeakerRadii::localAABB()
{
	// create the AABB from the radii we have and
	// make sure we take the biggest radius (even though the min radius doesn't make much sense)
	float radii = m_radii.getMin() > m_radii.getMax() ? m_radii.getMin() : m_radii.getMax();
	Vector3 radiiVector (radii, radii, radii);
	m_aabb_local = AABB(Vector3(0,0,0), radiiVector);
	return m_aabb_local;
}
#endif

void RenderableSpeakerRadiiWireframe::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;

    // Generate the three circles in axis-aligned planes
    const std::size_t NumSegments = 2;

    std::vector<ArbitraryMeshVertex> vertices;
    // Allocate space for 3 circles, each has NumSegments * 8 vertices
    vertices.resize(1 * (NumSegments << 3));

    draw_circle<RemapXYZ>(NumSegments, _radii.getMax(), vertices, 0);

    // Generate the indices, walking around the vertices
    std::vector<unsigned int> indices;
    auto numVertices = vertices.size();
    indices.reserve(numVertices << 1); // 2 indices per vertex

    for (auto i = 0; i < numVertices; ++i)
    {
        indices.push_back(i);
        indices.push_back((i + 1) % numVertices); // wrap around the last index to point at 0 again
    }

    // Move the points to their world position
    for (auto& vertex : vertices)
    {
        vertex.vertex += _origin;
    }

    RenderableGeometry::updateGeometry(render::GeometryType::Lines, vertices, indices);
}

#if 0
float RenderableSpeakerRadii::getMin() const
{
	return m_radii.getMin();
}

float RenderableSpeakerRadii::getMax() const
{
	return m_radii.getMax();
}
#endif

} // namespace
