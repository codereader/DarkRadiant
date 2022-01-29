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

    std::size_t drawCalls = 0;

    std::string toString() override
    {
        return fmt::format("Lights: {0} of {1} | Entities: {2} | Objects: {3} | Draws: {4}", 
            visibleLights, visibleLights + skippedLights, entities, objects, drawCalls);
    }
};

}
