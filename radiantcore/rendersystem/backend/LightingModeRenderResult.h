#pragma once

#include "irender.h"
#include "fmt/format.h"

namespace render
{

class LightingModeRenderResult :
    public IRenderResult
{
public:
    std::size_t visibleLights = 0;
    std::size_t skippedLights = 0;

    std::size_t entities = 0;
    std::size_t objects = 0;

    std::size_t depthDrawCalls = 0;
    std::size_t interactionDrawCalls = 0;
    std::size_t nonInteractionDrawCalls = 0;
    std::size_t shadowDrawCalls = 0;

    std::string toString() override
    {
        return fmt::format("Lights: {0}/{1} | Ents: {2} | Objs: {3} | Draws: D={4}|Int={5}|Bl={6}|Shdw={7}", 
            visibleLights, visibleLights + skippedLights, entities, objects, depthDrawCalls, 
            interactionDrawCalls, nonInteractionDrawCalls, shadowDrawCalls);
    }
};

}
