#include "ManipulationPivot.h"

#include "igrid.h"
#include "ilightnode.h"
#include "registry/registry.h"
#include "algorithm/General.h"
#include "selectionlib.h"

namespace selection
{

const std::string ManipulationPivot::RKEY_ENTITY_PIVOT_IS_ORIGIN = "user/ui/rotationPivotIsOrigin";
const std::string ManipulationPivot::RKEY_SNAP_ROTATION_PIVOT_TO_GRID = "user/ui/snapRotationPivotToGrid";
const std::string ManipulationPivot::RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES = "user/ui/defaultPivotLocationIgnoresLightVolumes";

ManipulationPivot::ManipulationPivot() :
	_entityPivotIsOrigin(false),
	_snapPivotToGrid(false),
	_needsRecalculation(true),
	_defaultPivotLocationIgnoresLightVolumes(false),
	_operationActive(false),
	_userLocked(false)
{}

void ManipulationPivot::initialise()
{
	_entityPivotIsOrigin = registry::getValue<bool>(RKEY_ENTITY_PIVOT_IS_ORIGIN);
	_snapPivotToGrid = registry::getValue<bool>(RKEY_SNAP_ROTATION_PIVOT_TO_GRID);
	_defaultPivotLocationIgnoresLightVolumes = registry::getValue<bool>(RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES);

	GlobalRegistry().signalForKey(RKEY_ENTITY_PIVOT_IS_ORIGIN).connect(
		sigc::mem_fun(this, &ManipulationPivot::onRegistryKeyChanged)
	);
	GlobalRegistry().signalForKey(RKEY_SNAP_ROTATION_PIVOT_TO_GRID).connect(
		sigc::mem_fun(this, &ManipulationPivot::onRegistryKeyChanged)
	);
	GlobalRegistry().signalForKey(RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES).connect(
		sigc::mem_fun(this, &ManipulationPivot::onRegistryKeyChanged)
	);
}

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
const Vector3& ManipulationPivot::getVector3()
{
	if (_needsRecalculation && !_operationActive && !_userLocked)
	{
		updateFromSelection();
	}

	return _pivot2World.t().getVector3();
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

void ManipulationPivot::applyTranslation(const Vector3& translation)
{
	// We apply translations on top of the starting point
	revertToStart();

	_pivot2World.translateBy(translation);

	if (_snapPivotToGrid)
	{
		// The resulting pivot should be grid-snapped
		_pivot2World.t().getVector3().snap(GlobalGrid().getGridSize());
	}
}

void ManipulationPivot::updateFromSelection()
{
	_needsRecalculation = false;
	_userLocked = false;

	Vector3 objectPivot;

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 1 && info.totalCount == 1 &&
		Node_getLightNode(GlobalSelectionSystem().ultimateSelected()))
	{
		// When a single light is selected, use the origin for rotation
		objectPivot = Node_getLightNode(GlobalSelectionSystem().ultimateSelected())->getSelectAABB().origin;
	}
	else if (info.entityCount == 1 && info.totalCount == 1 && _entityPivotIsOrigin)
	{
		// Test if a single entity is selected
		scene::INodePtr node = GlobalSelectionSystem().ultimateSelected();
		Entity* entity = Node_getEntity(node);

		if (entity != nullptr)
		{
			objectPivot = string::convert<Vector3>(entity->getKeyValue("origin"));
		}
	}
	else
	{
		// Create a local variable where the aabb information is stored
		AABB bounds;

		// Traverse through the selection and update the <bounds> variable
		if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent)
		{
			bounds = algorithm::getCurrentComponentSelectionBounds();
		}
		else
		{
			// Ignore light volumes for the pivot calculation
			bounds = algorithm::getCurrentSelectionBounds(!_defaultPivotLocationIgnoresLightVolumes);
		}

		// the <bounds> variable now contains the AABB of the selection, retrieve the origin
		objectPivot = bounds.origin;
	}

	if (_snapPivotToGrid)
	{
		objectPivot.snap(GlobalGrid().getGridSize());
	}

	// The pivot2world matrix is just a translation from the world origin (0,0,0) to the object pivot
	setFromMatrix(Matrix4::getTranslation(objectPivot));
}

void ManipulationPivot::onRegistryKeyChanged()
{
	_entityPivotIsOrigin = registry::getValue<bool>(RKEY_ENTITY_PIVOT_IS_ORIGIN);
	_snapPivotToGrid = registry::getValue<bool>(RKEY_SNAP_ROTATION_PIVOT_TO_GRID);
	_defaultPivotLocationIgnoresLightVolumes = registry::getValue<bool>(RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES);

	GlobalSelectionSystem().pivotChanged();
}

}
