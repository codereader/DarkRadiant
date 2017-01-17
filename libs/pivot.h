#pragma once

#include "ivolumetest.h"
#include "math/Matrix4.h"
#include "irenderable.h"
#include "render.h"

inline void billboard_viewplaneOriented(Matrix4& rotation, const Matrix4& world2screen)
{
#if 1
  rotation = Matrix4::getIdentity();
  Vector3 x(world2screen.x().getVector3().getNormalised());
  Vector3 y(world2screen.y().getVector3().getNormalised());
  Vector3 z(world2screen.z().getVector3().getNormalised());
  rotation.y().getVector3() = Vector3(x.y(), y.y(), z.y());
  rotation.z().getVector3() = -Vector3(x.z(), y.z(), z.z());
  rotation.x().getVector3() = rotation.y().getVector3().crossProduct(rotation.z().getVector3()).getNormalised();
  rotation.y().getVector3() = rotation.z().getVector3().crossProduct(rotation.x().getVector3());
#else
  Matrix4 screen2world(matrix4_full_inverse(world2screen));

  Vector3 near_(
    matrix4_transformed_vector4(
        screen2world,
        Vector4(0, 0, -1, 1)
      ).getProjected()
  );

  Vector3 far_(
      matrix4_transformed_vector4(
        screen2world,
        Vector4(0, 0, 1, 1)
      ).getProjected()
  );

  Vector3 up(
      matrix4_transformed_vector4(
        screen2world,
        Vector4(0, 1, -1, 1)
      ).getProjected()
  );

  rotation = Matrix4::getIdentity();
  rotation.y().getVector3() = (up - near_).getNormalised();
  rotation.z().getVector3() = (near_ - far_).getNormalised();
  rotation.x().getVector3() = rotation.y().getVector3().crossProduct(rotation.z().getVector3()).getNormalised();
  rotation.y().getVector3() = rotation.z().getVector3().crossProduct(rotation.x().getVector3());
#endif
}

inline void billboard_viewpointOriented(Matrix4& rotation, const Matrix4& world2screen)
{
  Matrix4 screen2world(world2screen.getFullInverse());

#if 1
  rotation = Matrix4::getIdentity();
  rotation.y().getVector3() = screen2world.y().getVector3().getNormalised();
  rotation.z().getVector3() = -screen2world.z().getVector3().getNormalised();
  rotation.x().getVector3() = rotation.y().getVector3().crossProduct(rotation.z().getVector3()).getNormalised();
  rotation.y().getVector3() = rotation.z().getVector3().crossProduct(rotation.x().getVector3());
#else
  Vector3 near_(
      matrix4_transformed_vector4(
        screen2world,
        Vector4(world2screen[12] / world2screen[15], world2screen[13] / world2screen[15], -1, 1)
      ).getProjected()
  );

  Vector3 far_(
      matrix4_transformed_vector4(
        screen2world,
        Vector4(world2screen[12] / world2screen[15], world2screen[13] / world2screen[15], 1, 1)
      ).getProjected()
  );

  Vector3 up(
      matrix4_transformed_vector4(
        screen2world,
        Vector4(world2screen[12] / world2screen[15], world2screen[13] / world2screen[15] + 1, -1, 1)
      ).getProjected()
  );

  rotation = Matrix4::getIdentity();
  rotation.y().getVector3() = (up - near_).getNormalised();
  rotation.z().getVector3() = (near_ - far_).getNormalised();
  rotation.x().getVector3() = rotation.y().getVector3().crossProduct(rotation.z().getVector3()).getNormalised();
  rotation.y().getVector3() = rotation.z().getVector3().crossProduct(rotation.x().getVector3());
#endif
}

/**
 * greebo: Returns a single matrix which transforms object coordinates to screen coordinates,
 * which is basically the same pipeline a vertex is sent through by openGL.
 * The 4 input matrices are just concatenated by multiplication, like this:
 * object2screen = device2screen * view2device * world2view * object2world
 */
inline Matrix4 constructObject2Screen(const Matrix4& object2world, const Matrix4& world2view, const Matrix4& view2device, const Matrix4& device2screen)
{
	return object2world.getPremultipliedBy(world2view).getPremultipliedBy(view2device).getPremultipliedBy(device2screen);
}

/**
 * greebo: Returns a matrix that transforming object coordinates to device coordinates [-1..+1].
 * The three input matrices are concatenated like this:
 * object2device = view2device * world2view * object2world
 */
inline Matrix4 constructObject2Device(const Matrix4& object2world, const Matrix4& world2view, const Matrix4& view2device)
{
	return object2world.getPremultipliedBy(world2view).getPremultipliedBy(view2device);
}

/**
 * Returns the full inverse of the object2device matrix, which transforms device coords
 * into object coordinates.
 */
inline Matrix4 constructDevice2Object(const Matrix4& object2world, const Matrix4& world2view, const Matrix4& view2device)
{
	return constructObject2Device(object2world, world2view, view2device).getFullInverse();
}

/**
 * greebo: The old code here was set up to calculate the scale out of the
 * pivot2screen (== object2screen) matrix and invert it like this:
 *
 //! S =  ( Inverse(Object2Screen *post ScaleOf(Object2Screen) ) *post Object2Screen
 *
 * Since the matrices are square and invertible, the above equation is equal to
 *
 * S = Inverse(Scaleof(Object2Screen)) *post Inverse(Object2Screen) *post Object2Screen
 *
 * the right two matrices cancel each other out to the identity transform, so all that remains is
 *
 * S = Inverse(ScaleOf(Object2Screen))
 *
 * The scale of the object2screen matrix can be extracted easily, and the inverse of a pure scale
 * matrix is basically inverting its diagonals, so the above can be equally constructed without 
 * inverting the whole 4x4 matrix.
 **/
inline Matrix4 getInverseScale(const Matrix4& transform)
{
#if 1
	return Matrix4::getScale(transform.getScale().getInversed());
#else 
	// Old code like described above
	// Extract the scale of the pivot2screen (transform) matrix and 
	// store that in another matrix
	Matrix4 pre_scale = Matrix4::getScale(transform.getScale());
	
	return transform.getMultipliedBy(pre_scale).getFullInverse().getMultipliedBy(transform);
#endif
}

// scale by (inverse) W
inline Matrix4 getPerspectiveScale(const Matrix4& pivot2screen)
{
	double tw = pivot2screen.tw();
	return Matrix4::getScale(Vector3(tw, tw, tw));
}

/**
 * greebo: Replacement for the above constructDevice2Manip. This produces a matrix
 * that is converting pivot space coordinates into normalised device coords.
 * The tz value of this matrix and its inverse can be used to convert mouse click coords 
 * back into pivot space.
 */
inline Matrix4 constructPivot2Device(const Matrix4& pivot2world, const VolumeTest& view)
{
	// First, apply the modelview on top of the pivot matrix [pivot => world => view]
	Matrix4 pivot2view = pivot2world.getPremultipliedBy(view.GetModelview());

	// Apply the projection matrix to get into clip space
	// The combined matrix is now performing this: [pivot => world => view => clip]
	Matrix4 pivot2clip = pivot2view.getPremultipliedBy(view.GetProjection());

	// Once we're into clip space, we can normalise coords by dividing by w.
	// This operation can be seen as some sort of perspective correction, 
	// this does nothing in orthographic projections where w == 1
	double inverseW = 1 / pivot2clip.tw();

	// Produce a matrix that is doing the 1/w division for us.
	// We need to divide the w component of the vectors too, so put inverseW in all 4 diagonals
	Matrix4 clip2device = Matrix4::getScale(Vector3(inverseW, inverseW, inverseW));
	clip2device.tw() = inverseW;

	// Apply this perspective division to clip space and we're done here
	return pivot2clip.getPremultipliedBy(clip2device);
}

/**
 * greebo: Generates a transformation that can be used to convert device coordinates back
 * into object space.
 */
inline Matrix4 constructDevice2Pivot(const Matrix4& pivot2world, const VolumeTest& view)
{
	return constructPivot2Device(pivot2world, view).getFullInverse();
}
