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
    std::size_t surfaces = 0;

    std::string toString() override
    {
        return fmt::format("Lights: {0} of {1} | Entities: {2} | Surfaces: {3}", visibleLights, skippedLights, entities, surfaces);
    }
};

}
