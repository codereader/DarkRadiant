#pragma once

#include "math/Matrix4.h"
#include "math/Vector3.h"

namespace selection
{

/**
 * Represents the anchor point for manipulation operations
 * in the scene. Usually this is defined by the origin of the
 * current selection AABB.
 *
 * Use the getMatrix4() method to acquire a pivot-to-world matrix.
 */
class ManipulationPivot
{
private:
	Matrix4 _pivot2World;

	// The untransformed pivot2world matrix
	// When an operation starts, the current state is saved here.
	// Since translations are relative to the starting point of the
	// operation, they are applied on top of the pivot2WorldStart.
	Matrix4 _pivot2WorldStart;

public:
	// Returns the pivot-to-world transform
	const Matrix4& getMatrix4() const;

	// Returns the position of the pivot point relative to origin
	const Vector3& getVector3() const;

	void setFromMatrix(const Matrix4& newPivot2World);

	// Call this before an operation is started, such that later
	// transformations can be applied on top of the correct starting point
	void beginOperation();

	// Reverts the matrix to the state it had at the beginning of the operation
	void revertToStart();

	void endOperation();

	void applyTranslation(const Vector3& translation);
};

}
