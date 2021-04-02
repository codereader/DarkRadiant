#pragma once

#include "ishaders.h"
#include "ishaderlayer.h"

namespace shaders
{

class FrobStageSetup
{
public:
    static bool IsPresent(const MaterialPtr& material)
    {


        return false;
    }

private:
    static std::string GetDiffuseMap(const MaterialPtr& material)
    {
        return std::string();
    }
};

}
