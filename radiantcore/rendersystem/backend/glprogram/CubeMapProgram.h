#pragma once

#include "GLSLProgramBase.h"
#include "math/Vector3.h"

namespace render
{

class CubeMapProgram: public GLSLProgramBase
{
    // Uniform/program-local parameter IDs.
    GLint _locViewOrigin = -1;

public:
    CubeMapProgram();

    /* GLProgram implementation */
    void enable() override;
    void disable() override;

    void setViewer(const Vector3& viewer);
};

} // namespace render
