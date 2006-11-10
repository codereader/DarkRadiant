#include "math/FloatTools.h"
#include "Manipulatables.h"
#include "Intersection.h"

void transform_local2object(Matrix4& object, const Matrix4& local, const Matrix4& local2object) {
  object = matrix4_multiplied_by_matrix4(
    matrix4_multiplied_by_matrix4(local2object, local),
    matrix4_full_inverse(local2object)
  );
}

void translation_local2object(Vector3& object, const Vector3& local, const Matrix4& local2object)
{
  object = matrix4_get_translation_vec3(
    matrix4_multiplied_by_matrix4(
      matrix4_translated_by_vec3(local2object, local),
      matrix4_full_inverse(local2object)
    )
  );
}

// ===============================================================================================

void RotateFree::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_sphere(m_start, device2manip, x, y);
    vector3_normalise(m_start);
}

void RotateFree::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;

    point_on_sphere(current, device2manip, x, y);
    vector3_normalise(current);

    m_rotatable.rotate(quaternion_for_unit_vectors(m_start, current));
}

// ===============================================================================================

void RotateAxis::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_sphere(m_start, device2manip, x, y);
    constrain_to_axis(m_start, m_axis);
}

/// \brief Converts current position to a normalised vector orthogonal to axis.
void RotateAxis::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;
    point_on_sphere(current, device2manip, x, y);
    constrain_to_axis(current, m_axis);

    m_rotatable.rotate(quaternion_for_axisangle(m_axis, angle_for_axis(m_start, current, m_axis)));
}

// ===============================================================================================

void TranslateAxis::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_axis(m_start, m_axis, device2manip, x, y);
}

void TranslateAxis::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;
    point_on_axis(current, m_axis, device2manip, x, y);
    current = m_axis * distance_for_axis(m_start, current, m_axis);

    translation_local2object(current, current, manip2object);
    vector3_snap(current, GetGridSize());

    m_translatable.translate(current);
}

// ===============================================================================================

void TranslateFree::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_plane(m_start, device2manip, x, y);
}

void TranslateFree::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;
    point_on_plane(current, device2manip, x, y);
    current = current - m_start;

    translation_local2object(current, current, manip2object);
    vector3_snap(current, GetGridSize());
    
    m_translatable.translate(current);
}

// ===============================================================================================

void ScaleAxis::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_axis(m_start, m_axis, device2manip, x, y);
}

void ScaleAxis::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;
    point_on_axis(current, m_axis, device2manip, x, y);
    Vector3 delta = current - m_start;

    translation_local2object(delta, delta, manip2object);
    vector3_snap(delta, GetGridSize());
    
    Vector3 start(vector3_snapped(m_start, GetGridSize()));
    Vector3 scale(
      start[0] == 0 ? 1 : 1 + delta[0] / start[0],
      start[1] == 0 ? 1 : 1 + delta[1] / start[1],
      start[2] == 0 ? 1 : 1 + delta[2] / start[2]
    );
    m_scalable.scale(scale);
}

// ===============================================================================================

void ScaleFree::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_plane(m_start, device2manip, x, y);
}

void ScaleFree::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;
    point_on_plane(current, device2manip, x, y);
    Vector3 delta = current - m_start;

    translation_local2object(delta, delta, manip2object);
    vector3_snap(delta, GetGridSize());
    
    Vector3 start(vector3_snapped(m_start, GetGridSize()));
    Vector3 scale(
      start[0] == 0 ? 1 : 1 + delta[0] / start[0],
      start[1] == 0 ? 1 : 1 + delta[1] / start[1],
      start[2] == 0 ? 1 : 1 + delta[2] / start[2]
    );
    m_scalable.scale(scale);
}
