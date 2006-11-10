#include "Intersection.h"

void point_for_device_point(Vector3& point, const Matrix4& device2object, const float x, const float y, const float z) {
  // transform from normalised device coords to object coords
  point = matrix4_transformed_vector4(device2object, Vector4(x, y, z, 1)).getProjected();
}

void ray_for_device_point(Ray& ray, const Matrix4& device2object, const float x, const float y) {
    // point at x, y, zNear
  point_for_device_point(ray.origin, device2object, x, y, -1);

  // point at x, y, zFar
  point_for_device_point(ray.direction, device2object, x, y, 1);

  // construct ray
  ray.direction -= ray.origin;
  vector3_normalise(ray.direction);
}

bool sphere_intersect_ray(const Vector3& origin, float radius, const Ray& ray, Vector3& intersection) {
  intersection = origin - ray.origin;
  const double a = intersection.dot(ray.direction);
  const double d = radius * radius - (intersection.dot(intersection) - a * a);

  if(d > 0)
  {
    intersection = ray.origin + ray.direction * (a - sqrt(d));
    return true;
  }
  else
  {
    intersection = ray.origin + ray.direction*a;
    return false;
  }
}

void ray_intersect_ray(const Ray& ray, const Ray& other, Vector3& intersection) {
  intersection = ray.origin - other.origin;
  //float a = 1;//ray.direction.dot(ray.direction);        // always >= 0
  double dot = ray.direction.dot(other.direction);
  //float c = 1;//other.direction.dot(other.direction);        // always >= 0
  double d = ray.direction.dot(intersection);
  double e = other.direction.dot(intersection);
  double D = 1 - dot*dot;//a*c - dot*dot;       // always >= 0

  if (D < 0.000001) {
    // the lines are almost parallel
    intersection = other.origin + other.direction*e;
  }
  else {
    intersection = other.origin + other.direction*((e - dot*d) / D);
  }    
}

const Vector3 g_origin(0, 0, 0);
const float g_radius = 64;

void point_on_sphere(Vector3& point, const Matrix4& device2object, const float x, const float y) {
  Ray ray;
  ray_for_device_point(ray, device2object, x, y);
  sphere_intersect_ray(g_origin, g_radius, ray, point);
}

void point_on_axis(Vector3& point, const Vector3& axis, const Matrix4& device2object, const float x, const float y) {
  Ray ray;
  ray_for_device_point(ray, device2object, x, y);
  ray_intersect_ray(ray, Ray(Vector3(0, 0, 0), axis), point);
}

void point_on_plane(Vector3& point, const Matrix4& device2object, const float x, const float y) {
  Matrix4 object2device(matrix4_full_inverse(device2object));
  point = matrix4_transformed_vector4(device2object, Vector4(x, y, object2device[14] / object2device[15], 1)).getProjected();
}

//! a and b are unit vectors .. a and b must be orthogonal to axis .. returns angle in radians
float angle_for_axis(const Vector3& a, const Vector3& b, const Vector3& axis) {
  if(axis.dot(a.crossProduct(b)) > 0.0)
    return angle_between(a, b);
  else
    return -angle_between(a, b);
}

float distance_for_axis(const Vector3& a, const Vector3& b, const Vector3& axis) {
  return static_cast<float>(b.dot(axis) - a.dot(axis));
}

