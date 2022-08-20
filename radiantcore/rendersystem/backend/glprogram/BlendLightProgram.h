#pragma once

#include "GLSLProgramBase.h"

namespace render
{

class BlendLightProgram :
    public GLSLProgramBase
{
public:
    void create() override;
    void enable() override;
    void disable() override;
};

}
