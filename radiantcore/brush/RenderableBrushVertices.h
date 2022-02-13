#pragma once

#include "render/VertexCb.h"
#include "render/RenderableGeometry.h"

class Brush;

namespace brush
{

/**
 * Maintains all vertices of a brush (vertices, edges, face centroids)
 * depending on the component mode.
 */
class RenderableBrushVertices :
    public render::RenderableGeometry
{
private:
    Brush& _brush;
    const std::vector<Vector3>& _selectedVertices;

    // The mode this renderable has been configured for
    selection::ComponentSelectionMode _mode;

    bool _updateNeeded;

public:
    RenderableBrushVertices(Brush& brush, const std::vector<Vector3>& selectedVertices) :
        _brush(brush),
        _selectedVertices(selectedVertices),
        _updateNeeded(true)
    {}

    void queueUpdate()
    {
        _updateNeeded = true;
    }

    // Configure this renderable to display the vertices matching the given mode
    void setComponentMode(selection::ComponentSelectionMode mode)
    {
        if (_mode == mode) return;

        _mode = mode;
        queueUpdate();
    }

protected:
    void updateGeometry() override;
};

}
