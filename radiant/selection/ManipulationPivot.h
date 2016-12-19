#pragma once

#include "math/Matrix4.h"

namespace selection
{

/**
 * Represents the anchor point for manipulation operations
 * in the scene. Usually this is defined by the origin of the
 * current selection AABB.
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
	const Matrix4& getMatrix4() const
	{
		return _pivot2World;
	}

	const Vector3& getVector3() const
	{
		return _pivot2World.t().getVector3();
	}

	void setFromMatrix(const Matrix4& newPivot2World)
	{
		_pivot2World = newPivot2World;
	}

	// Call this before an operation is started, such that later
	// transformations can be applied on top of the correct starting point
	void beginOperation()
	{
		_pivot2WorldStart = _pivot2World;
	}

	// Reverts the matrix to the state it had at the beginning of the operation
	void revertToStart()
	{
		_pivot2World = _pivot2WorldStart;
	}

	void endOperation()
	{
		_pivot2WorldStart = _pivot2World;
	}

	void applyTranslation(const Vector3& translation)
	{
		// We apply translations on top of the starting point
		revertToStart();

		_pivot2World.translateBy(translation);
	}
};

}
