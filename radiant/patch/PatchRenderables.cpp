#include "PatchRenderables.h"

void RenderablePatchWireframe::render(const RenderInfo& info) const
{
    // No colour changing
    glDisableClientState(GL_COLOR_ARRAY);
    if (info.checkFlag(RENDER_VERTEX_COLOUR))
    {
        glColor3f(1, 1, 1);
    }

    if (m_tess.vertices.empty()) return;

    const std::vector<ArbitraryMeshVertex>& patchVerts = m_tess.vertices;

    // Vertex buffer to receive and render vertices
    VertexBuffer_T currentVBuf;

    std::size_t firstIndex = 0;
    for(std::size_t i = 0; i <= m_tess.curveTreeV.size(); ++i)
    {
        currentVBuf.addBatch(patchVerts.begin() + firstIndex,
                             m_tess.m_nArrayWidth);

        if(i == m_tess.curveTreeV.size()) break;

        if (!m_tess.curveTreeV[i]->isLeaf())
        {
            currentVBuf.addBatch(
                patchVerts.begin() + GLint(m_tess.curveTreeV[i]->index),
                m_tess.m_nArrayWidth
            );
        }

        firstIndex += (m_tess.arrayHeight[i]*m_tess.m_nArrayWidth);
    }

    const ArbitraryMeshVertex* p = &patchVerts.front();
    std::size_t uStride = m_tess.m_nArrayWidth;
    for(std::size_t i = 0; i <= m_tess.curveTreeU.size(); ++i)
    {
        currentVBuf.addBatch(p, m_tess.m_nArrayHeight, uStride);

        if(i == m_tess.curveTreeU.size()) break;

        if(!m_tess.curveTreeU[i]->isLeaf())
        {
            currentVBuf.addBatch(
                patchVerts.begin() + m_tess.curveTreeU[i]->index,
                m_tess.m_nArrayHeight, uStride
            );
        }

        p += m_tess.arrayWidth[i];
    }

    // Render all vertex batches
    _vertexBuf.replaceData(currentVBuf);
    _vertexBuf.renderAllBatches(GL_LINE_STRIP);
}

void RenderablePatchFixedWireframe::render(const RenderInfo& info) const
{
    if (m_tess.vertices.empty() || m_tess.indices.empty()) return;

    // No colour changing
    glDisableClientState(GL_COLOR_ARRAY);
    if (info.checkFlag(RENDER_VERTEX_COLOUR))
    {
        glColor3f(1, 1, 1);
    }

    // Create a VBO and add the vertex data
    VertexBuffer_T currentVBuf;
    currentVBuf.addVertices(m_tess.vertices.begin(), m_tess.vertices.end());

    // Submit index batches
    const RenderIndex* strip_indices = &m_tess.indices.front();
    for (std::size_t i = 0;
         i < m_tess.m_numStrips;
         i++, strip_indices += m_tess.m_lenStrips)
    {
        currentVBuf.addIndexBatch(strip_indices, m_tess.m_lenStrips);
    }

    // Render all index batches
    _vertexBuf.replaceData(currentVBuf);
    _vertexBuf.renderAllBatches(GL_QUAD_STRIP);
}

RenderablePatchSolid::RenderablePatchSolid(PatchTesselation& tess) :
    m_tess(tess)
{ }

void RenderablePatchSolid::render(const RenderInfo& info) const
{
    if (m_tess.vertices.empty() || m_tess.indices.empty()) return;

    if (!info.checkFlag(RENDER_BUMP))
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    // No colour changing
    glDisableClientState(GL_COLOR_ARRAY);
    if (info.checkFlag(RENDER_VERTEX_COLOUR))
    {
        glColor3f(1, 1, 1);
    }

    // Add vertex geometry to vertex buffer
    VertexBuffer_T currentVBuf;
    currentVBuf.addVertices(m_tess.vertices.begin(), m_tess.vertices.end());

    // Submit indices
    const RenderIndex* strip_indices = &m_tess.indices.front();
    for(std::size_t i = 0;
        i<m_tess.m_numStrips;
        i++, strip_indices += m_tess.m_lenStrips)
    {
        currentVBuf.addIndexBatch(strip_indices, m_tess.m_lenStrips);
    }

    // Render all batches
    _vertexBuf.replaceData(currentVBuf);
    _vertexBuf.renderAllBatches(GL_QUAD_STRIP, info.checkFlag(RENDER_BUMP));
}
