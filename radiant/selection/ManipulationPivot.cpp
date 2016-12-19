#include "ManipulationPivot.h"

namespace selection
{

// Returns the pivot-to-world transform
const Matrix4& ManipulationPivot::getMatrix4() const
{
	return _pivot2World;
}

// Returns the position of the pivot point relative to origin
const Vector3& ManipulationPivot::getVector3() const
{
	return _pivot2World.t().getVector3();
}

void ManipulationPivot::setFromMatrix(const Matrix4& newPivot2World)
{
	_pivot2World = newPivot2World;
}

// Call this before an operation is started, such that later
// transformations can be applied on top of the correct starting point
void ManipulationPivot::beginOperation()
{
	_pivot2WorldStart = _pivot2World;
}

// Reverts the matrix to the state it had at the beginning of the operation
void ManipulationPivot::revertToStart()
{
	_pivot2World = _pivot2WorldStart;
}

void ManipulationPivot::endOperation()
{
	_pivot2WorldStart = _pivot2World;
}

void ManipulationPivot::applyTranslation(const Vector3& translation)
{
	// We apply translations on top of the starting point
	revertToStart();

	_pivot2World.translateBy(translation);
}

}
