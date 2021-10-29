#include "ManipulationPivot.h"

namespace selection
{

ManipulationPivot::ManipulationPivot() :
	_needsRecalculation(true),
	_operationActive(false),
	_userLocked(false)
{}

// Returns the pivot-to-world transform
const Matrix4& ManipulationPivot::getMatrix4()
{
	if (_needsRecalculation && !_operationActive && !_userLocked)
	{
		updateFromSelection();
	}

	return _pivot2World;
}

// Returns the position of the pivot point relative to origin
Vector3 ManipulationPivot::getVector3()
{
	if (_needsRecalculation && !_operationActive && !_userLocked)
	{
		updateFromSelection();
	}

	return _pivot2World.translation();
}

void ManipulationPivot::setFromMatrix(const Matrix4& newPivot2World)
{
	_pivot2World = newPivot2World;
}

void ManipulationPivot::setNeedsRecalculation(bool needsRecalculation)
{
	_needsRecalculation = needsRecalculation;
}

void ManipulationPivot::setUserLocked(bool locked)
{
	_userLocked = locked;
}

// Call this before an operation is started, such that later
// transformations can be applied on top of the correct starting point
void ManipulationPivot::beginOperation()
{
	_pivot2WorldStart = _pivot2World;
	_operationActive = true;
}

// Reverts the matrix to the state it had at the beginning of the operation
void ManipulationPivot::revertToStart()
{
	_pivot2World = _pivot2WorldStart;
}

void ManipulationPivot::endOperation()
{
	_pivot2WorldStart = _pivot2World;
	_operationActive = false;
}

void ManipulationPivot::cancelOperation()
{
    revertToStart();
    _operationActive = false;
}

void ManipulationPivot::applyTranslation(const Vector3& translation)
{
	// We apply translations on top of the starting point
	revertToStart();

	_pivot2World.translateBy(translation);
}

}
