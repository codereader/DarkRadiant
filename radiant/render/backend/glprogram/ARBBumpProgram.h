#ifndef ARBBUMPPROGRAM_H_
#define ARBBUMPPROGRAM_H_

#include "GLProgramAttributes.h"
#include "iglrender.h"
#include "math/matrix.h"

namespace render {

class ARBBumpProgram 
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

#ifdef RADIANT_USE_GLSL

    // Program object identifier
    GLuint _programObj;

#else

    // Vertex and fragment program identifiers
    GLuint m_vertex_program;
    GLuint m_fragment_program;

#endif

public:

    /* GLProgram implementation */
    void create();
    void destroy();
    void enable();
    void disable();

    // Set render pass parameters
    void applyRenderParams(const Vector3& viewer, 
                           const Matrix4& localToWorld, 
                           const Vector3& origin, 
                           const Vector3& colour, 
                           const Matrix4& world2light,
                           float ambientFactor);
};

} // namespace render

#endif /*ARBBUMPPROGRAM_H_*/
