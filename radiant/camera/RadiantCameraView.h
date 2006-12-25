#ifndef RADIANTCAMERAVIEW_H_
#define RADIANTCAMERAVIEW_H_

#include "icamera.h"
#include "math/frustum.h"

#include "view.h"
#include "Camera.h"

const Matrix4 g_radiant2opengl(
  0,-1, 0, 0,
  0, 0, 1, 0,
 -1, 0, 0, 0,
  0, 0, 0, 1
);

const Matrix4 g_opengl2radiant(
  0, 0,-1, 0,
 -1, 0, 0, 0,
  0, 1, 0, 0,
  0, 0, 0, 1
);

inline Matrix4 projection_for_camera(float near_z, float far_z, float fieldOfView, int width, int height) {
  const float half_width = static_cast<float>(near_z * tan(degrees_to_radians(fieldOfView * 0.5)));
  const float half_height = half_width * (static_cast<float>(height) / static_cast<float>(width));

  return matrix4_frustum(
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
  View* m_view;
  Callback m_update;
public:
  RadiantCameraView(Camera& camera, View* view, const Callback& update) : m_camera(camera), m_view(view), m_update(update)
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
    matrix4_multiply_by_matrix4(m_camera.modelview, g_radiant2opengl);
    matrix4_affine_invert(m_camera.modelview);
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

#endif /*RADIANTCAMERAVIEW_H_*/
