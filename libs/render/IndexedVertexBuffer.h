#pragma once

#include "render.h"
#include <GL/glew.h>
#include "render/VBO.h"

#include "GLProgramAttributes.h"

namespace render
{

/**
 * \brief
 * Receiver of indexed vertex geometry for rendering
 *
 * An IndexedVertexBuffer stores a pool of vertex data, and receives batches of
 * indices into the vertex buffer which are rendered with glDrawElements. It
 * functions much like \a VertexBuffer except for indexed geometry rather than
 * raw vertex geometry.
 */
template<typename Vertex_T>
class IndexedVertexBuffer final
{
public:
    typedef std::vector<Vertex_T> Vertices;
    typedef std::vector<RenderIndex> Indices;

private:

    typedef VertexTraits<Vertex_T> Traits;

    // OpenGL VBO
    mutable GLuint _vertexVBO;
    mutable GLuint _indexVBO;

    // Vertex and index storage
    Vertices _vertices;
    Indices _indices;

    // Batches of indices (start index and count)
    struct Batch
    {
        std::size_t start;
        std::size_t size;
    };

    // Batches
    std::vector<Batch> _batches;

private:

    void initialiseVBOs() const
    {
        _vertexVBO = makeVBOFromArray(GL_ARRAY_BUFFER, _vertices);
        _indexVBO = makeVBOFromArray(GL_ELEMENT_ARRAY_BUFFER, _indices);
    }

public:

    /// Construct an empty IndexedVertexBuffer
    IndexedVertexBuffer()
    : _vertexVBO(0), _indexVBO(0)
    { }

    /// Destroy resources
    ~IndexedVertexBuffer()
    {
        deleteVBO(_vertexVBO);
        deleteVBO(_indexVBO);
    }

    /**
     * \brief
     * Add vertices to the IndexedVertexBuffer
     *
     * Unlike with VertexBuffer, adding vertices to an IndexedVertexBuffer does
     * not create a batch. It just adds vertices to the common vertex pool.
     */
    template<typename Iter_T>
    void addVertices(Iter_T begin, Iter_T end)
    {
        std::copy(begin, end, std::back_inserter(_vertices));
    }

    typename Vertices::size_type getNumVertices() const
    {
        return _vertices.size();
    }

    /// Add a batch of indices
    template<typename Iter_T>
    void addIndexBatch(Iter_T begin, std::size_t count)
    {
        if (count < 1)
        {
            throw std::logic_error("Batch must contain at least one index");
        }

        // Create the new batch
        Batch newBatch = { _indices.size(), count };
        _batches.push_back(newBatch);

        // Copy the indices
        _indices.reserve(_indices.size() + count);
        for (Iter_T i = begin; i < begin + count; ++i)
        {
            _indices.push_back(*i);
        }
    }

    /// Add a batch of indices
    template<typename Iter_T>
    void addIndicesToLastBatch(Iter_T begin, std::size_t count, Indices::value_type offset = 0)
    {
        if (_batches.empty())
        {
            addIndexBatch(begin, count);
            return;
        }

        if (count < 1)
        {
            throw std::logic_error("Batch must contain at least one index");
        }

        auto& batch = *_batches.rbegin();
        batch.size += count;

        // Append the indices
        _indices.reserve(_indices.size() + count);

        for (Iter_T i = begin; i < begin + count; ++i)
        {
            _indices.push_back(*i + offset);
        }

        // Invalidate the vertex and index buffers
        deleteVBO(_vertexVBO);
        deleteVBO(_indexVBO);
    }

    /**
     * \brief
     * Replace all data with that from another IndexedVertexBuffer
     *
     * \see VertexBuffer::replaceData
     */
    void replaceData(const IndexedVertexBuffer& other)
    {
        replaceVBODataIfPossible(GL_ARRAY_BUFFER, _vertexVBO,
                                 _vertices, other._vertices);
        replaceVBODataIfPossible(GL_ELEMENT_ARRAY_BUFFER, _indexVBO,
                                 _indices, other._indices);

        _vertices = other._vertices;
        _indices = other._indices;
        _batches = other._batches;
    }

    /**
     * \brief
     * Render all batches with given primitive type
     *
     * \param renderBump
     * True if tangent and bitangent vectors should be submitted for bump map
     * rendering. If false, only regular normal vectors will be submitted. In
     * all cases the pointers will only be set if carried by the particular
     * vertex type.
     */
    void renderAllBatches(GLenum primitiveType, bool renderBump = false) const
    {
        if (_vertexVBO == 0 || _indexVBO == 0)
        {
            initialiseVBOs();
        }

        glBindBuffer(GL_ARRAY_BUFFER, _vertexVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexVBO);

        // Vertex pointer includes whole vertex buffer
        const GLsizei STRIDE = sizeof(Vertex_T);
        glVertexPointer(3, GL_DOUBLE, STRIDE, Traits::VERTEX_OFFSET());

        // Set other pointers as necessary
        if (Traits::hasTexCoord())
        {
            if (renderBump)
            {
                glVertexAttribPointer(
                    ATTR_TEXCOORD, 2, GL_DOUBLE, GL_FALSE,
                    STRIDE, Traits::TEXCOORD_OFFSET()
                );
            }
            else
            {
                glTexCoordPointer(2, GL_DOUBLE, STRIDE,
                                  Traits::TEXCOORD_OFFSET());
            }
        }
        if (Traits::hasNormal())
        {
            if (renderBump)
            {
                glVertexAttribPointer(
                    ATTR_NORMAL, 3, GL_DOUBLE, GL_FALSE,
                    STRIDE, Traits::NORMAL_OFFSET()
                );
            }
            else
            {
                glNormalPointer(GL_DOUBLE, STRIDE, Traits::NORMAL_OFFSET());
            }
        }
        if (Traits::hasTangents() && renderBump)
        {
            glVertexAttribPointer(ATTR_TANGENT, 3, GL_DOUBLE, GL_FALSE,
                                  STRIDE, Traits::TANGENT_OFFSET());
            glVertexAttribPointer(ATTR_BITANGENT, 3, GL_DOUBLE, GL_FALSE,
                                  STRIDE, Traits::BITANGENT_OFFSET());
        }

        // Render each batch of indices
        for (typename std::vector<Batch>::const_iterator i = _batches.begin();
             i != _batches.end();
             ++i)
        {
            glDrawElements(
                primitiveType, GLint(i->size), RenderIndexTypeID,
                reinterpret_cast<const GLvoid*>(
                    i->start * sizeof(Indices::value_type)
                )
            );
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
};

}
