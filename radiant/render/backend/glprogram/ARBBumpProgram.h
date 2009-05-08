#ifndef ARBBUMPPROGRAM_H_
#define ARBBUMPPROGRAM_H_

#include "GLProgramAttributes.h"
#include "iglrender.h"
#include "math/matrix.h"

namespace render {

/* CONSTANTS */
namespace {
    const char* BUMP_VP_FILENAME = "interaction_vp.arb";
    const char* BUMP_FP_FILENAME = "interaction_fp.arb";
}

class ARBBumpProgram 
: public GLProgram
{
private:

	// The value all lights should be scaled by, obtained from the game description
	double _lightScale;

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
