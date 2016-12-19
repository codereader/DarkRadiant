#include "ManipulationPivot.h"

#include "algorithm/General.h"
#include "selectionlib.h"

namespace selection
{

namespace
{
	const std::string RKEY_ENTITY_PIVOT_IS_ORIGIN = "user/ui/rotationPivotIsOrigin";
}

ManipulationPivot::ManipulationPivot() :
	_entityPivotIsOrigin(false)
{}

void ManipulationPivot::initialise()
{
	_entityPivotIsOrigin = registry::getValue<bool>(RKEY_ENTITY_PIVOT_IS_ORIGIN);

	GlobalRegistry().signalForKey(RKEY_ENTITY_PIVOT_IS_ORIGIN).connect(
		sigc::mem_fun(this, &ManipulationPivot::onRegistryKeyChanged)
	);
}

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

void ManipulationPivot::updateFromSelection()
{
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
			bounds = algorithm::getCurrentSelectionBounds();
		}

		// the <bounds> variable now contains the AABB of the selection, retrieve the origin
		objectPivot = bounds.origin;
	}

	// Snap the pivot point to the grid (greebo: disabled this (issue #231))
	//vector3_snap(objectPivot, GlobalGrid().getGridSize());

	// The pivot2world matrix is just a translation from the world origin (0,0,0) to the object pivot
	setFromMatrix(Matrix4::getTranslation(objectPivot));
}

void ManipulationPivot::onRegistryKeyChanged()
{
	_entityPivotIsOrigin = registry::getValue<bool>(RKEY_ENTITY_PIVOT_IS_ORIGIN);

	GlobalSelectionSystem().pivotChanged();
}

}
