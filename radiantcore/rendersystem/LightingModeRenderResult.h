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

    std::string toString() override
    {
        return fmt::format("Lights: {0} of {1}", visibleLights, skippedLights);
    }
};

}
