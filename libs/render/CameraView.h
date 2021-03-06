#pragma once

#include "icameraview.h"
#include "math/Matrix4.h"

// Undef stupid global definitions from minwindef.h
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

namespace camera
{

const Matrix4 g_radiant2opengl = Matrix4::byColumns(
    0, -1, 0, 0,
    0, 0, 1, 0,
    -1, 0, 0, 0,
    0, 0, 0, 1
);

const Matrix4 g_opengl2radiant = Matrix4::byColumns(
    0, 0, -1, 0,
    -1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 1
);

inline Matrix4 calculateModelViewMatrix(const Vector3& origin, const Vector3& angles)
{
    auto modelview = Matrix4::getIdentity();

    // roll, pitch, yaw
    Vector3 radiant_eulerXYZ(0, -angles[CAMERA_PITCH], angles[CAMERA_YAW]);

    modelview.translateBy(origin);
    modelview.rotateByEulerXYZDegrees(radiant_eulerXYZ);
    modelview.multiplyBy(g_radiant2opengl);
    modelview.invert();

    return modelview;
}

inline Matrix4 getProjectionForFrustum(double left, double right, double bottom,
                                       double top, double near,
                                       double far)
{
    return Matrix4::byColumns(
        (2*near) / (right-left), 0, 0, 0,
        0, (2*near) / (top-bottom), 0, 0,
        (right+left) / (right-left), (top+bottom) / (top-bottom), -(far+near) / (far-near), -1,
        0, 0, -(2*far*near) / (far-near), 0
    );
}

inline Matrix4 calculateProjectionMatrix(float near_z, float far_z, float fieldOfView, int width, int height)
{
    const auto half_width = near_z * tan(degrees_to_radians(fieldOfView * 0.5));
    const auto half_height = half_width * (static_cast<double>(height) / static_cast<double>(width));

    return getProjectionForFrustum(-half_width, half_width, -half_height,
                                   half_height, near_z, far_z);
}

}
