#ifndef INTERSECTION_H_
#define INTERSECTION_H_

#include "math/line.h"

/* greebo: Intersection methods. Needed to perform selectionTests
 * 
 * I know, this is not a terribly helpful comment, but I haven't looked into this yet. 
 */

void point_for_device_point(Vector3& point, const Matrix4& device2object, const float x, const float y, const float z);
void ray_for_device_point(Ray& ray, const Matrix4& device2object, const float x, const float y);

bool sphere_intersect_ray(const Vector3& origin, float radius, const Ray& ray, Vector3& intersection);
void ray_intersect_ray(const Ray& ray, const Ray& other, Vector3& intersection);

void point_on_sphere(Vector3& point, const Matrix4& device2object, const float x, const float y);
void point_on_axis(Vector3& point, const Vector3& axis, const Matrix4& device2object, const float x, const float y);
void point_on_plane(Vector3& point, const Matrix4& device2object, const float x, const float y);

//! a and b are unit vectors .. returns angle in radians
// greebo: I don't know if this is the fastest way of doing this, a simple 
// arccos( a.dotproduct(b) ) should be enough, but: "if it isn't broken, don't fix it"
inline float angle_between(const Vector3& a, const Vector3& b) {
  return static_cast<float>(2.0 * atan2( (a-b).getLength(), (a+b).getLength() ));
}

//! axis is a unit vector
inline void constrain_to_axis(Vector3& vec, const Vector3& axis) {
  vec = (vec + axis*(-vec.dot(axis))).getNormalised();
}

//! a and b are unit vectors .. a and b must be orthogonal to axis .. returns angle in radians
float angle_for_axis(const Vector3& a, const Vector3& b, const Vector3& axis);

float distance_for_axis(const Vector3& a, const Vector3& b, const Vector3& axis);

#endif /*INTERSECTION_H_*/
