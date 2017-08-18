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

	// Use a single Entity's "origin" keyvalue as pivot
	bool _entityPivotIsOrigin;

	// Whether to snap the pivot to grid after movement
	bool _snapPivotToGrid;

	// "dirty" flag
	bool _needsRecalculation;

	// Whether to consider light volumes when calculating the selection bounds
	bool _defaultPivotLocationIgnoresLightVolumes;

	// During operations, we want to block pivot recalculations
	bool _operationActive;

	// "User has modified pivot"-flag, used to block pivot recalculations
	bool _userLocked;

public:
	static const std::string RKEY_ENTITY_PIVOT_IS_ORIGIN;
	static const std::string RKEY_SNAP_ROTATION_PIVOT_TO_GRID;
	static const std::string RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES;

	ManipulationPivot();

	void initialise();

	// Returns the pivot-to-world transform
	const Matrix4& getMatrix4();

	// Returns the position of the pivot point relative to origin
	const Vector3& getVector3();

	void setFromMatrix(const Matrix4& newPivot2World);

	// Set the dirty flag of the matrix, this will trigger 
	// an updateFromSelection() next time getMatrix4() is called
	void setNeedsRecalculation(bool needsRecalculation);

	// If the user has placed the pivot manually, we want to refrain
	// from recalculating it automatically.
	void setUserLocked(bool locked);

	// Call this before an operation is started, such that later
	// transformations can be applied on top of the correct starting point
	void beginOperation();

	// Reverts the matrix to the state it had at the beginning of the operation
	void revertToStart();

	void endOperation();

	void applyTranslation(const Vector3& translation);

	// Rescans the selection and calculates the pivot afresh,
	// respecting the currently active settings
	void updateFromSelection();

private:
	void onRegistryKeyChanged();
};

}
