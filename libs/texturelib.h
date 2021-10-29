#pragma once

#include "math/Vector3.h"
#include "math/Vector2.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include <vector>
#include <limits.h>

#include "iimage.h"
#include "ishaders.h"

// Promotes the given 3x3 texture projection matrix to the 4x4 type
inline Matrix4 getMatrix4FromTextureMatrix(const Matrix3& matrix3)
{
    auto matrix4 = Matrix4::getIdentity();

    matrix4.xx() = matrix3.xx();
    matrix4.xy() = matrix3.xy();
    matrix4.yy() = matrix3.yy();
    matrix4.yx() = matrix3.yx();

    // Z => T
    matrix4.tx() = matrix3.zx();
    matrix4.ty() = matrix3.zy();

    return matrix4;
}

inline Matrix3 getTextureMatrixFromMatrix4(const Matrix4& matrix4)
{
    auto matrix3 = Matrix3::getIdentity();

    matrix3.xx() = matrix4.xx();
    matrix3.xy() = matrix4.xy();
    matrix3.yy() = matrix4.yy();
    matrix3.yx() = matrix4.yx();

    // T => Z
    matrix3.zx() = matrix4.tx();
    matrix3.zy() = matrix4.ty();

    return matrix3;
}

enum ProjectionAxis {
	eProjectionAxisX = 0,
	eProjectionAxisY = 1,
	eProjectionAxisZ = 2,
};

const double ProjectionAxisEpsilon = 0.0001;

inline bool projectionaxis_better(double axis, double other) {
	return fabs(axis) > fabs(other) + ProjectionAxisEpsilon;
}

/// \brief Texture axis precedence: Z > X > Y
inline ProjectionAxis projectionaxis_for_normal(const Vector3& normal) {
	return (projectionaxis_better(normal[eProjectionAxisY], normal[eProjectionAxisX]))
		? (projectionaxis_better(normal[eProjectionAxisY], normal[eProjectionAxisZ]))
			? eProjectionAxisY
			: eProjectionAxisZ
		: (projectionaxis_better(normal[eProjectionAxisX], normal[eProjectionAxisZ]))
			? eProjectionAxisX
			: eProjectionAxisZ;
}

/*!
\brief Construct a transform from XYZ space to ST space (3d to 2d).
This will be one of three axis-aligned spaces, depending on the surface normal.
NOTE: could also be done by swapping values.
*/
inline void Normal_GetTransform(const Vector3& normal, Matrix4& transform) {
	switch (projectionaxis_for_normal(normal)) {
		case eProjectionAxisZ:
			transform[0]  =  1;
			transform[1]  =  0;
			transform[2]  =  0;

			transform[4]  =  0;
			transform[5]  =  1;
			transform[6]  =  0;

			transform[8]  =  0;
			transform[9]  =  0;
			transform[10] =  1;
		break;
		case eProjectionAxisY:
			transform[0]  =  1;
			transform[1]  =  0;
			transform[2]  =  0;

			transform[4]  =  0;
			transform[5]  =  0;
			transform[6]  = -1;

			transform[8]  =  0;
			transform[9]  =  1;
			transform[10] =  0;
		break;
		case eProjectionAxisX:
			transform[0]  =  0;
			transform[1]  =  0;
			transform[2]  =  1;

			transform[4]  =  1;
			transform[5]  =  0;
			transform[6]  =  0;

			transform[8]  =  0;
			transform[9]  =  1;
			transform[10] =  0;
		break;
	}
	transform[3] = transform[7] = transform[11] = transform[12] = transform[13] = transform[14] = 0;
	transform[15] = 1;
}

/* greebo: This method calculates the normalised texture basis vectors of the texture plane 
 * as defined by <normal>.
 * 
 * Why is this needed? To calculate the texture coords of a brush winding, 
 * the texture projection matrix is used to transform the 3D vertex coords into 2D UV space. 
 * But *before* this happens the world system has to be rotated to align with 
 * the face's UV space. We only need the face normal to calculate the UV base vectors.
 * 
 * There are two special cases coded into this method dealing with normals pointing 
 * straight up or straight down - in theses cases the world XY plane is
 * rotated by 90 degrees.
 * 
 * The idTech4 game engine is doing the same, even though the algorithm for the
 * method "ComputeAxisBase" is implemented differently, based on atan2 instead
 * of if-else and cross-products. Note that this behaviour seems to be different
 * from Quake3, which appears to use one of six possibly axis-aligned projection bases.
 * 
 * The output basis vectors will be normalised.
 * 
 * If the normal vector points to the z-direction, the basis vectors are part
 * of the xy-plane: texS = <0,1,0> and texT = <1,0,0>
 *
 * If normal vector points to the negative z-direction, the above case applies, but with
 * the x-direction inversed: texS = <0,1,0> and texT = <-1,0,0> (note the minus)
 *
 * (I assume that the rotation of the vectors is to make images appear upright instead
 * of vertically flipped for faces that are pointing towards -z or +z.)
 * 
 * If none of the two above cases apply, the basis is calculated via cross products
 * that result in vectors perpendicular to <normal>. These lie within the plane
 * that is defined by the normal vector itself.
 *
 * Note: the vector <normal> MUST be normalised for this to function correctly.
 */
inline void ComputeAxisBase(const Vector3& normal, Vector3& texS, Vector3& texT)
{
	static const Vector3 up(0, 0, 1);
	static const Vector3 down(0, 0, -1);

	if (math::isNear(normal, up, 1e-6)) // straight up?
	{
		texS = Vector3(0, 1, 0);
		texT = Vector3(1, 0, 0);
	}
	else if (math::isNear(normal, down, 1e-6)) // straight down?
	{
		texS = Vector3(0, 1, 0);
		texT = Vector3(-1, 0, 0);
	}
	else
	{
		texS = normal.cross(up).getNormalised();
		texT = normal.cross(texS).getNormalised();
		texS = -texS;
	}
}

/* greebo: This returns the basis vectors of the texture (plane) space needed for projection.
 * The vectors are normalised and stored within the basis matrix <basis>
 * as column vectors.
 *
 * Note: the normal vector MUST be normalised already when this function is called,
 * but this should be fulfilled as it represents a FacePlane vector (which is usually normalised)
 */
inline Matrix4 getBasisTransformForNormal(const Vector3& normal)
{
    Vector3 texS, texT;
    ComputeAxisBase(normal, texS, texT);

    Matrix4 basis = Matrix4::getIdentity();
    basis.setXCol(texS);
    basis.setYCol(texT);
    basis.setZCol(normal);

    // At this point the basis matrix contains three column vectors that are
    // perpendicular to each other.

    // The x-line of <basis> contains the <texS> basis vector (within the face plane)
    // The y-line of <basis> contains the <texT> basis vector (within the face plane)
    // The z-line of <basis> contains the <normal> basis vector (perpendicular to the face plane)

    // invert this matrix and return
    return basis.getTransposed(); 
}

/* greebo: this is used to calculate the directions the patch is "flattened" in.
 * If one of the patch bases is parallel or anti-parallel to the <faceNormal> it cannot
 * be projected onto the facePlane, so a new orthogonal vector is taken as direction instead.
 *
 * This prevents the patch from disappearing and the texture from being infinetly stretched in such cases.
 *
 * @returns: This returns two normalised vectors that are orthogonal to the face plane normal and point
 * into the direction of the patch orientation. */
inline void getVirtualPatchBase(const Vector3& widthVector, const Vector3& heightVector,
								const Vector3& faceNormal, Vector3& widthBase, Vector3& heightBase)
{
	bool widthVectorIsParallel = math::isParallel(widthVector, faceNormal);
	bool heightVectorIsParallel = math::isParallel(heightVector, faceNormal);

	if (widthVectorIsParallel) {
		// Calculate a orthogonal width vector
		widthBase = faceNormal.cross(heightVector).getNormalised();
	}
	else {
		// Project the vector onto the faceplane (this is the width direction)
		widthBase = (widthVector - faceNormal*(faceNormal*widthVector)).getNormalised();
	}

	if (heightVectorIsParallel) {
		// Calculate a orthogonal height vector
		heightBase = faceNormal.cross(widthVector).getNormalised();
	}
	else {
		// Project the vector onto the faceplane (this is the height direction)
		heightBase = (heightVector - faceNormal*(faceNormal*heightVector)).getNormalised();
	}
}

// handles degenerate cases, just in case library atan2 doesn't
inline double arctangent_yx(double y, double x) {
	if (fabs(x) > 1.0E-6) {
		return atan2(y, x);
	}
	else if (y > 0) {
		return c_half_pi;
	}
	else {
		return -c_half_pi;
	}
}

// Returns the index of the one edge which points "most" into the given direction, <direction> should be normalised
inline std::size_t findBestEdgeForDirection(const Vector2& direction, const std::vector<Vector2>& edges)
{
	double best = std::numeric_limits<double>::lowest();
	std::size_t bestIndex = 0;

	for (std::size_t i = 0; i < edges.size(); ++i)
	{
		double dot = direction.dot(edges[i]);

		if (dot <= best) continue;

		// Found a new best edge
		bestIndex = i;
		best = dot;
	}

	return bestIndex;
}
