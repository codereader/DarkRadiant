#pragma once

#include "LightVertexInstanceSet.h"
#include "render/RenderableGeometry.h"

namespace entity
{

class LightNode;

// The small diamond representing at the light's origin
// This is using the Triangle geometry type such that we can see 
// the half-transparent (red) overlay when the light is selected
class RenderableLightOctagon :
    public render::RenderableGeometry
{
private:
    const LightNode& _light;
    bool _needsUpdate;
    float _alpha;

public:
    RenderableLightOctagon(const LightNode& light, float alpha) :
        _light(light),
        _needsUpdate(true),
        _alpha(alpha)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

protected:
    void updateGeometry() override;
};

// The wireframe showing the light volume of the light
// which is either a box (point light) or a frustum/cone (projected)
class RenderableLightVolume :
    public render::RenderableGeometry
{
private:
    const LightNode& _light;
    bool _needsUpdate;

public:
    RenderableLightVolume(const LightNode& light) :
        _light(light),
        _needsUpdate(true)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

protected:
    void updateGeometry() override;

private:
    void updatePointLightVolume();
    void updateProjectedLightVolume();
};

// All manipulatable vertices, with the colour
// corresponding to their selection status
class RenderableLightVertices :
    public render::RenderableGeometry
{
private:
    const LightNode& _light;
    const LightVertexInstanceSet& _instances;
    const Projected<bool>& _useFlags;

    bool _needsUpdate;
    selection::ComponentSelectionMode _mode;

public:
    RenderableLightVertices(const LightNode& light, 
                            const LightVertexInstanceSet& instances, 
                            const Projected<bool>& useFlags) :
        _light(light),
        _instances(instances),
        _useFlags(useFlags),
        _needsUpdate(true),
        _mode(selection::ComponentSelectionMode::Default)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

    void setComponentMode(selection::ComponentSelectionMode mode)
    {
        if (_mode == mode) return;

        _mode = mode;
        queueUpdate();
    }

protected:
    void updateGeometry() override;
};

} // namespace entity
