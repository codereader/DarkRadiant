#pragma once

#include "ManipulationPivot.h"

namespace selection
{

/**
 * ManipulationPivot specialisation tailored to suit the needs
 * of the main map selection manipulations.
 */
class SceneManipulationPivot final :
    public ManipulationPivot
{
private:
    // Whether to snap the pivot to grid after movement
    bool _snapPivotToGrid;

    // Use a single Entity's "origin" keyvalue as pivot
    bool _entityPivotIsOrigin;

    // Whether to consider light volumes when calculating the selection bounds
    bool _defaultPivotLocationIgnoresLightVolumes;

public:
    static const std::string RKEY_ENTITY_PIVOT_IS_ORIGIN;
    static const std::string RKEY_SNAP_ROTATION_PIVOT_TO_GRID;
    static const std::string RKEY_DEFAULT_PIVOT_LOCATION_IGNORES_LIGHT_VOLUMES;

    SceneManipulationPivot();

    void initialise();

    void applyTranslation(const Vector3& translation) override;

    void updateFromSelection() override;

private:
    void onRegistryKeyChanged();
};

}
