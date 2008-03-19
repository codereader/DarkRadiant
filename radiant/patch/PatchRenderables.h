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
  {
  }
  void render(RenderStateFlags state) const
  {
    {
  #if NV_DRIVER_BUG
      glVertexPointer(3, GL_DOUBLE, 0, 0);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 0);
  #endif

      std::size_t n = 0;
      glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->vertex);
      for(std::size_t i = 0; i <= m_tess.m_curveTreeV.size(); ++i)
      {
        glDrawArrays(GL_LINE_STRIP, GLint(n), GLsizei(m_tess.m_nArrayWidth));

        if(i == m_tess.m_curveTreeV.size()) break;

        if(!BezierCurveTree_isLeaf(m_tess.m_curveTreeV[i]))
          glDrawArrays(GL_LINE_STRIP, GLint(m_tess.m_curveTreeV[i]->index), GLsizei(m_tess.m_nArrayWidth));

        n += (m_tess.m_arrayHeight[i]*m_tess.m_nArrayWidth);
      
      }
    }

    {
      const ArbitraryMeshVertex* p = m_tess.m_vertices.data();
      std::size_t n = m_tess.m_nArrayWidth * sizeof(ArbitraryMeshVertex);
      for(std::size_t i = 0; i <= m_tess.m_curveTreeU.size(); ++i)
      {
        glVertexPointer(3, GL_DOUBLE, GLsizei(n), &p->vertex);
        glDrawArrays(GL_LINE_STRIP, 0, GLsizei(m_tess.m_nArrayHeight));

        if(i == m_tess.m_curveTreeU.size()) break;

        if(!BezierCurveTree_isLeaf(m_tess.m_curveTreeU[i]))
        {
          glVertexPointer(3, GL_DOUBLE, GLsizei(n), &(m_tess.m_vertices.data() + (m_tess.m_curveTreeU[i]->index))->vertex);
          glDrawArrays(GL_LINE_STRIP, 0, GLsizei(m_tess.m_nArrayHeight));
        }

        p += m_tess.m_arrayWidth[i];
      }
    }
  }
};

class RenderablePatchFixedWireframe : public OpenGLRenderable
{
  PatchTesselation& m_tess;
public:
  RenderablePatchFixedWireframe(PatchTesselation& tess) : m_tess(tess)
  {
  }
  void render(RenderStateFlags state) const
  {
    glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->vertex);
    const RenderIndex* strip_indices = m_tess.m_indices.data();
    for(std::size_t i = 0; i<m_tess.m_numStrips; i++, strip_indices += m_tess.m_lenStrips)
    {
      glDrawElements(GL_QUAD_STRIP, GLsizei(m_tess.m_lenStrips), RenderIndexTypeID, strip_indices);
    }
  }
};

class RenderablePatchSolid : public OpenGLRenderable
{
  PatchTesselation& m_tess;
public:
  RenderablePatchSolid(PatchTesselation& tess) : m_tess(tess)
  {
  }
  void RenderNormals() const;
  void render(RenderStateFlags state) const
  {
#if 0
    if((state & RENDER_FILL) == 0)
    {
      RenderablePatchWireframe(m_tess).render(state);
    }
    else
#endif
    {
      if((state & RENDER_BUMP) != 0)
      {
        /*if(GlobalShaderCache().useShaderLanguage())
        {
          glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->normal);
          glVertexAttribPointerARB(c_attr_TexCoord0, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->texcoord);
          glVertexAttribPointerARB(c_attr_Tangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->tangent);
          glVertexAttribPointerARB(c_attr_Binormal, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->bitangent);
        }
        else
        {*/
          glVertexAttribPointerARB(11, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->normal);
          glVertexAttribPointerARB(8, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->texcoord);
          glVertexAttribPointerARB(9, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->tangent);
          glVertexAttribPointerARB(10, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->bitangent);
        /*}*/
      }
      else
      {
        glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->normal);
        glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->texcoord);
      }
      glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_tess.m_vertices.data()->vertex);
      const RenderIndex* strip_indices = m_tess.m_indices.data();
      for(std::size_t i = 0; i<m_tess.m_numStrips; i++, strip_indices += m_tess.m_lenStrips)
      {
        glDrawElements(GL_QUAD_STRIP, GLsizei(m_tess.m_lenStrips), RenderIndexTypeID, strip_indices);
      }
    }
 
#if defined(_DEBUG)
    RenderNormals();
#endif
  }
};

#endif /*PATCH_RENDERABLES_H_*/
