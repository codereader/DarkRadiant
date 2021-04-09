#pragma once

#include "GLSLProgramBase.h"

namespace render
{

class GLSLDepthFillAlphaProgram :
    public GLSLProgramBase
{
public:
    void create() override;
};

}
