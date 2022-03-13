#include "SceneRenderer.h"

#include "igl.h"
#include "OpenGLState.h"

namespace render
{

namespace
{
    // Polygon stipple pattern
    const GLubyte POLYGON_STIPPLE_PATTERN[132] =
    {
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
        0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55
    };
}

void SceneRenderer::setupViewMatrices(const IRenderView& view)
{
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(view.GetProjection());

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(view.GetModelview());
}

void SceneRenderer::setupState(OpenGLState& state)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // global settings that are not set in renderstates
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glPolygonOffset(-1, 1);

    // Set polygon stipple pattern from constant
    glPolygonStipple(POLYGON_STIPPLE_PATTERN);

    // Vertex Arrays stay enabled throughout the pass
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    if (GLEW_VERSION_1_3)
    {
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
    }

    glUseProgram(0);

    glDisableVertexAttribArray(GLProgramAttribute::Position);
    glDisableVertexAttribArray(GLProgramAttribute::TexCoord);
    glDisableVertexAttribArray(GLProgramAttribute::Tangent);
    glDisableVertexAttribArray(GLProgramAttribute::Bitangent);
    glDisableVertexAttribArray(GLProgramAttribute::Normal);
    glDisableVertexAttribArray(GLProgramAttribute::Colour);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set up initial GL state. This MUST MATCH the defaults in the OpenGLState
    // object, otherwise required state changes may not occur.
    glLineStipple(state.m_linestipple_factor, state.m_linestipple_pattern);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_FLAT);
    glDisable(GL_DEPTH_TEST);

    // RENDER_DEPTHWRITE defaults to 0
    glDepthMask(GL_FALSE);

    // RENDER_MASKCOLOUR defaults to 0
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDisable(GL_ALPHA_TEST);

    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_POLYGON_STIPPLE);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glDisable(GL_POLYGON_OFFSET_FILL); // greebo: otherwise tiny gap lines between brushes are visible

    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(1, 1, 1, 1);
    glDepthFunc(state.getDepthFunc());
    glAlphaFunc(GL_ALWAYS, 0);
    glLineWidth(1);
    glPointSize(1);

    glHint(GL_FOG_HINT, GL_NICEST);
    glDisable(GL_FOG);
}

void SceneRenderer::cleanupState()
{
    if (GLEW_ARB_shader_objects)
    {
        glUseProgram(0);
    }

    glDisableVertexAttribArrayARB(GLProgramAttribute::Position);
    glDisableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Tangent);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Bitangent);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Normal);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Colour);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glPopAttrib();
}

}
