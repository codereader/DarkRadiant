#ifndef PATCH_RENDERABLES_H_
#define PATCH_RENDERABLES_H_

#include "igl.h"
#include "PatchTesselation.h"

/* greebo: These are the renderables that are used in the PatchNode/Patch class to
 * draw the patch onto the screen.
 */

class RenderablePatchWireframe : public OpenGLRenderable
{
    PatchTesselation& m_tess;
public:

    RenderablePatchWireframe(PatchTesselation& tess) : m_tess(tess)
    { }

    void render(const RenderInfo& info) const
    {
        // No colour changing
        glDisableClientState(GL_COLOR_ARRAY);
        if (info.checkFlag(RENDER_VERTEX_COLOUR))
        {
            glColor3f(1, 1, 1);
        }

        {
  #if NV_DRIVER_BUG
      glVertexPointer(3, GL_DOUBLE, 0, 0);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 0);
  #endif

	  if (m_tess.vertices.empty()) return;

      std::size_t n = 0;
      glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_tess.vertices.front().vertex);
      for(std::size_t i = 0; i <= m_tess.curveTreeV.size(); ++i)
      {
        glDrawArrays(GL_LINE_STRIP, GLint(n), GLsizei(m_tess.m_nArrayWidth));

        if(i == m_tess.curveTreeV.size()) break;

        if (!m_tess.curveTreeV[i]->isLeaf())
		{
			glDrawArrays(GL_LINE_STRIP, GLint(m_tess.curveTreeV[i]->index), GLsizei(m_tess.m_nArrayWidth));
		}

        n += (m_tess.arrayHeight[i]*m_tess.m_nArrayWidth);

      }
        }

    {
      const ArbitraryMeshVertex* p = &m_tess.vertices.front();
      std::size_t n = m_tess.m_nArrayWidth * sizeof(ArbitraryMeshVertex);
      for(std::size_t i = 0; i <= m_tess.curveTreeU.size(); ++i)
      {
        glVertexPointer(3, GL_DOUBLE, GLsizei(n), &p->vertex);
        glDrawArrays(GL_LINE_STRIP, 0, GLsizei(m_tess.m_nArrayHeight));

        if(i == m_tess.curveTreeU.size()) break;

		if(!m_tess.curveTreeU[i]->isLeaf())
        {
			glVertexPointer(3, GL_DOUBLE, GLsizei(n), &m_tess.vertices[m_tess.curveTreeU[i]->index].vertex);
          glDrawArrays(GL_LINE_STRIP, 0, GLsizei(m_tess.m_nArrayHeight));
        }

        p += m_tess.arrayWidth[i];
      }
        }
    }
};

class RenderablePatchFixedWireframe : public OpenGLRenderable
{
    PatchTesselation& m_tess;
public:

    RenderablePatchFixedWireframe(PatchTesselation& tess) : m_tess(tess)
    { }

    void render(const RenderInfo& info) const
    {
        if (m_tess.vertices.empty() || m_tess.indices.empty()) return;

        // No colour changing
        glDisableClientState(GL_COLOR_ARRAY);
        if (info.checkFlag(RENDER_VERTEX_COLOUR))
        {
            glColor3f(1, 1, 1);
        }

        glVertexPointer(3,
                        GL_DOUBLE,
                        sizeof(ArbitraryMeshVertex),
                        &m_tess.vertices.front().vertex);

        const RenderIndex* strip_indices = &m_tess.indices.front();
        for (std::size_t i = 0;
             i < m_tess.m_numStrips;
             i++, strip_indices += m_tess.m_lenStrips)
        {
            glDrawElements(GL_QUAD_STRIP,
                           GLsizei(m_tess.m_lenStrips),
                           RenderIndexTypeID,
                           strip_indices);
        }
    }
};

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

//#define PATCHES_USE_VBO

class RenderablePatchSolid :
	public OpenGLRenderable
{
	PatchTesselation& m_tess;

	// Vertex buffer objects
	GLuint _vboData;
	GLuint _vboIdx;

public:
	RenderablePatchSolid(PatchTesselation& tess);

	// Updates rendering structures
	void update();

	// Implementation is in Patch.cpp
	void RenderNormals() const;

	void render(const RenderInfo& info) const;
};

#endif /*PATCH_RENDERABLES_H_*/
