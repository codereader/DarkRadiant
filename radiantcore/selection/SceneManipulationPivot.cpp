#include "SceneManipulationPivot.h"

#include "iselection.h"
#include "ilightnode.h"
#include "ientity.h"
#include "igrid.h"
#include "selectionlib.h"
#include "registry/registry.h"
#include "selection/algorithm/General.h"

namespace selection
{

const std::string SceneManipulationPivot::RKEY_ENTITY_PIVOT_IS_ORIGIN = "user/ui/rotationPivotIsOrigin";
const std::string SceneManipulationPivot::RKEY_SNAP_ROTATION_PIVOT_TO_GRID = "user/ui/snapRotationPivotToGrid";
const std::string SceneManipulationPivot::RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES = "user/ui/defaultPivotLocationIgnoresLightVolumes";

SceneManipulationPivot::SceneManipulationPivot() :
    _entityPivotIsOrigin(false),
    _snapPivotToGrid(false),
    _defaultPivotLocationIgnoresLightVolumes(false)
{}

void SceneManipulationPivot::initialise()
{
    _entityPivotIsOrigin = registry::getValue<bool>(RKEY_ENTITY_PIVOT_IS_ORIGIN);
    _snapPivotToGrid = registry::getValue<bool>(RKEY_SNAP_ROTATION_PIVOT_TO_GRID);
    _defaultPivotLocationIgnoresLightVolumes = registry::getValue<bool>(RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES);

    GlobalRegistry().signalForKey(RKEY_ENTITY_PIVOT_IS_ORIGIN).connect(
        sigc::mem_fun(this, &SceneManipulationPivot::onRegistryKeyChanged)
    );
    GlobalRegistry().signalForKey(RKEY_SNAP_ROTATION_PIVOT_TO_GRID).connect(
        sigc::mem_fun(this, &SceneManipulationPivot::onRegistryKeyChanged)
    );
    GlobalRegistry().signalForKey(RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES).connect(
        sigc::mem_fun(this, &SceneManipulationPivot::onRegistryKeyChanged)
    );
}

void SceneManipulationPivot::applyTranslation(const Vector3& translation)
{
    ManipulationPivot::applyTranslation(translation);

    if (_snapPivotToGrid)
    {
        // The resulting pivot should be grid-snapped
        _pivot2World.setTranslation(
            _pivot2World.translation().getSnapped(GlobalGrid().getGridSize())
        );
    }
}

void SceneManipulationPivot::updateFromSelection()
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
        if (GlobalSelectionSystem().getSelectionMode() == SelectionMode::Component)
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

void SceneManipulationPivot::onRegistryKeyChanged()
{
    _entityPivotIsOrigin = registry::getValue<bool>(RKEY_ENTITY_PIVOT_IS_ORIGIN);
    _snapPivotToGrid = registry::getValue<bool>(RKEY_SNAP_ROTATION_PIVOT_TO_GRID);
    _defaultPivotLocationIgnoresLightVolumes = registry::getValue<bool>(RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES);

    GlobalSelectionSystem().pivotChanged();
}

}
