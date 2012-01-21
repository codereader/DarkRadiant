#pragma once

#include "GLProgramAttributes.h"
#include "iglprogram.h"
#include "igl.h"

namespace render
{

class GLSLBumpProgram
: public GLProgram
{
	// The value all lights should be scaled by, obtained from the game description
	double _lightScale;

    // Uniform/program-local parameter IDs.
    int _locLightOrigin;
    int _locLightColour;
    int _locViewOrigin;
    int _locLightScale;
    int _locAmbientFactor;
    int _locVColScale;
    int _locVColOffset;

    // Program object identifier
    GLuint _programObj;

public:

    /* GLProgram implementation */
    void create();
    void destroy();
    void enable();
    void disable();
    void applyRenderParams(const Vector3& viewer,
                           const Matrix4& localToWorld,
                           const Params&);
};

} // namespace render

