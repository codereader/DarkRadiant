#include "PatchRenderables.h"

void RenderablePatchWireframe::render(const RenderInfo& info) const
{
    // No colour changing
    glDisableClientState(GL_COLOR_ARRAY);
    if (info.checkFlag(RENDER_VERTEX_COLOUR))
    {
        glColor3f(1, 1, 1);
    }

    if (_tess.vertices.empty()) return;

    if (_needsUpdate)
    {
        _needsUpdate = false;

        // Create a VBO and add the vertex data
        VertexBuffer_T currentVBuf;
        currentVBuf.addVertices(_tess.vertices.begin(), _tess.vertices.end());

        // Submit index batches
        const RenderIndex* strip_indices = &_tess.indices.front();
        for (std::size_t i = 0;
            i < _tess.m_numStrips;
            i++, strip_indices += _tess.m_lenStrips)
        {
            currentVBuf.addIndexBatch(strip_indices, _tess.m_lenStrips);
        }

        // Render all index batches
        _vertexBuf.replaceData(currentVBuf);
#if 0
        const std::vector<ArbitraryMeshVertex>& patchVerts = _tess.vertices;

        // Vertex buffer to receive and render vertices
        VertexBuffer_T currentVBuf;

        std::size_t firstIndex = 0;
        for (std::size_t i = 0; i <= _tess.curveTreeV.size(); ++i)
        {
            currentVBuf.addBatch(patchVerts.begin() + firstIndex,
                _tess.m_nArrayWidth);

            if (i == _tess.curveTreeV.size()) break;

            if (!_tess.curveTreeV[i]->isLeaf())
            {
                currentVBuf.addBatch(
                    patchVerts.begin() + GLint(_tess.curveTreeV[i]->index),
                    _tess.m_nArrayWidth
                    );
            }

            firstIndex += (_tess.arrayHeight[i] * _tess.m_nArrayWidth);
        }

        const ArbitraryMeshVertex* p = &patchVerts.front();
        std::size_t uStride = _tess.m_nArrayWidth;
        for (std::size_t i = 0; i <= _tess.curveTreeU.size(); ++i)
        {
            currentVBuf.addBatch(p, _tess.m_nArrayHeight, uStride);

            if (i == _tess.curveTreeU.size()) break;

            if (!_tess.curveTreeU[i]->isLeaf())
            {
                currentVBuf.addBatch(
                    patchVerts.begin() + _tess.curveTreeU[i]->index,
                    _tess.m_nArrayHeight, uStride
                    );
            }

            p += _tess.arrayWidth[i];
        }

        // Render all vertex batches
        _vertexBuf.replaceData(currentVBuf);
#endif
    }

    _vertexBuf.renderAllBatches(GL_QUAD_STRIP);
}

void RenderablePatchWireframe::queueUpdate()
{
    _needsUpdate = true;
}

void RenderablePatchFixedWireframe::render(const RenderInfo& info) const
{
    if (_tess.vertices.empty() || _tess.indices.empty()) return;

    // No colour changing
    glDisableClientState(GL_COLOR_ARRAY);
    if (info.checkFlag(RENDER_VERTEX_COLOUR))
    {
        glColor3f(1, 1, 1);
    }

    if (_needsUpdate)
    {
        _needsUpdate = false;

        // Create a VBO and add the vertex data
        VertexBuffer_T currentVBuf;
        currentVBuf.addVertices(_tess.vertices.begin(), _tess.vertices.end());

        // Submit index batches
        const RenderIndex* strip_indices = &_tess.indices.front();
        for (std::size_t i = 0;
            i < _tess.m_numStrips;
            i++, strip_indices += _tess.m_lenStrips)
        {
            currentVBuf.addIndexBatch(strip_indices, _tess.m_lenStrips);
        }

        // Render all index batches
        _vertexBuf.replaceData(currentVBuf);
    }

    _vertexBuf.renderAllBatches(GL_QUAD_STRIP);
}

void RenderablePatchFixedWireframe::queueUpdate()
{
    _needsUpdate = true;
}

RenderablePatchSolid::RenderablePatchSolid(PatchTesselation& tess) :
    _tess(tess),
    _needsUpdate(true)
{}

void RenderablePatchSolid::render(const RenderInfo& info) const
{
    if (_tess.vertices.empty() || _tess.indices.empty()) return;

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

    if (_needsUpdate)
    {
        _needsUpdate = false;

        // Add vertex geometry to vertex buffer
        VertexBuffer_T currentVBuf;
        currentVBuf.addVertices(_tess.vertices.begin(), _tess.vertices.end());

        // Submit indices
        const RenderIndex* strip_indices = &_tess.indices.front();
        for (std::size_t i = 0;
            i < _tess.m_numStrips;
            i++, strip_indices += _tess.m_lenStrips)
        {
            currentVBuf.addIndexBatch(strip_indices, _tess.m_lenStrips);
        }

        // Render all batches
        _vertexBuf.replaceData(currentVBuf);
    }

    _vertexBuf.renderAllBatches(GL_QUAD_STRIP, info.checkFlag(RENDER_BUMP));
}

void RenderablePatchSolid::queueUpdate()
{
    _needsUpdate = true;
}
