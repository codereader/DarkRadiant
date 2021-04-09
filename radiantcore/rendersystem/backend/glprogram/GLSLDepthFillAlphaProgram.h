#pragma once

#include "GLSLProgramBase.h"

namespace render
{

class GLSLDepthFillAlphaProgram :
    public GLSLProgramBase
{
private:
    GLint _locAlphaTest;
    
public:
    void create() override;
    void enable() override;
    void disable() override;

    void applyAlphaTest(float alphaTest);
};

}
