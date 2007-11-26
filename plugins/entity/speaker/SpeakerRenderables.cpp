#include "SpeakerRenderables.h"

// the drawing functions

void sphereDrawFill(const Vector3& origin, float radius, int sides)
{
  if (radius <= 0)
    return;

  const double dt = c_2pi / static_cast<double>(sides);
  const double dp = c_pi / static_cast<double>(sides);

  glBegin(GL_TRIANGLES);
  for (int i = 0; i <= sides - 1; ++i)
  {
    for (int j = 0; j <= sides - 2; ++j)
    {
      const double t = i * dt;
      const double p = (j * dp) - (c_pi / 2.0);

      {
        Vector3 v(origin + vector3_for_spherical(t, p) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + vector3_for_spherical(t, p + dp) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + vector3_for_spherical(t + dt, p + dp) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + vector3_for_spherical(t, p) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + vector3_for_spherical(t + dt, p + dp) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + vector3_for_spherical(t + dt, p) * radius);
        glVertex3dv(v);
      }
    }
  }

  {
    const double p = (sides - 1) * dp - (c_pi / 2.0);
    for (int i = 0; i <= sides - 1; ++i)
    {
      const double t = i * dt;

      {
        Vector3 v(origin + vector3_for_spherical(t, p) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + vector3_for_spherical(t + dt, p + dp) * radius);
        glVertex3dv(v);
      }

      {
        Vector3 v(origin + vector3_for_spherical(t + dt, p) * radius);
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
      double ds = sin((i * 2 * c_pi) / sides);
      double dc = cos((i * 2 * c_pi) / sides);

      glVertex3d(
        static_cast<float>(origin[0] + radius * dc),
        static_cast<float>(origin[1] + radius * ds),
        origin[2]
      );
    }

    glEnd();
  }

  {
    glBegin(GL_LINE_LOOP);

    for (int i = 0; i <= sides; i++)
    {
      double ds = sin((i * 2 * c_pi) / sides);
      double dc = cos((i * 2 * c_pi) / sides);

      glVertex3d(
        static_cast<float>(origin[0] + radius * dc),
        origin[1],
        static_cast<float>(origin[2] + radius * ds)
      );
    }

    glEnd();
  }

  {
    glBegin(GL_LINE_LOOP);

    for (int i = 0; i <= sides; i++)
    {
      double ds = sin((i * 2 * c_pi) / sides);
      double dc = cos((i * 2 * c_pi) / sides);

      glVertex3d(
        origin[0],
        static_cast<float>(origin[1] + radius * dc),
        static_cast<float>(origin[2] + radius * ds)
      );
    }

    glEnd();
  }
}

void speakerDrawRadiiWire(const Vector3& origin, const SoundRadii rad)
{
  if(rad.getMin() > 0)
    sphereDrawWire(origin, rad.getMin(), 24);
  if(rad.getMax() > 0)
    sphereDrawWire(origin, rad.getMax(), 24);
}

void speakerDrawRadiiFill(const Vector3& origin, const SoundRadii rad)
{
  if(rad.getMin() > 0)
    sphereDrawFill(origin, rad.getMin(), 16);
  if(rad.getMax() > 0)
    sphereDrawFill(origin, rad.getMax(), 16);
}

namespace entity {

void RenderSpeakerRadii::render(RenderStateFlags state) const {
	//draw the radii of speaker based on speaker shader/radii keys
	if((state & RENDER_FILL) != 0) {
		speakerDrawRadiiFill(m_origin, m_radii);
	}
	else {
		speakerDrawRadiiWire(m_origin, m_radii);
	}
}

const AABB& RenderSpeakerRadii::localAABB() {
	// create the AABB from the radii we have and
	// make sure we take the biggest radius (even though the min radius doesn't make much sense)
	float radii = m_radii.getMin() > m_radii.getMax() ? m_radii.getMin() : m_radii.getMax();
	Vector3 radiiVector (radii, radii, radii);
	m_aabb_local = AABB (m_aabb_local.getOrigin(), radiiVector);
	return m_aabb_local;
}

} // namespace entity
