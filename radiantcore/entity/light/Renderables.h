#pragma once

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

public:
    RenderableLightOctagon(const LightNode& light) :
        _light(light),
        _needsUpdate(true)
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

} // namespace entity
