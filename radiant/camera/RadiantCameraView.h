#pragma once

#include "icamera.h"
#include "math/Frustum.h"

#include "render/View.h"
#include "Camera.h"

const Matrix4 g_radiant2opengl = Matrix4::byColumns(
  0,-1, 0, 0,
  0, 0, 1, 0,
 -1, 0, 0, 0,
  0, 0, 0, 1
);

const Matrix4 g_opengl2radiant = Matrix4::byColumns(
  0, 0,-1, 0,
 -1, 0, 0, 0,
  0, 1, 0, 0,
  0, 0, 0, 1
);

inline Matrix4 projection_for_camera(float near_z, float far_z, float fieldOfView, int width, int height) {
  const float half_width = near_z * tan(degrees_to_radians(fieldOfView * 0.5f));
  const float half_height = half_width * (static_cast<float>(height) / static_cast<float>(width));

  return Matrix4::getProjectionForFrustum(
    -half_width,
    half_width,
    -half_height,
    half_height,
    near_z,
    far_z
  );
}

class RadiantCameraView : public CameraView
{
  Camera& m_camera;
  render::View* m_view;
  Callback m_update;
public:
  RadiantCameraView(Camera& camera, render::View* view, const Callback& update) : m_camera(camera), m_view(view), m_update(update)
  {
  }
  void update()
  {
    m_view->Construct(m_camera.projection, m_camera.modelview, m_camera.width, m_camera.height);
    m_update();
  }
  void setModelview(const Matrix4& modelview)
  {
    m_camera.modelview = modelview;
    m_camera.modelview.multiplyBy(g_radiant2opengl);
    m_camera.modelview.invert();
    m_camera.updateVectors();
    update();
  }
  void setFieldOfView(float fieldOfView)
  {
    float farClip = m_camera.getFarClipPlane();
    m_camera.projection = projection_for_camera(farClip / 4096.0f, farClip, fieldOfView, m_camera.width, m_camera.height);
    update();
  }
};
