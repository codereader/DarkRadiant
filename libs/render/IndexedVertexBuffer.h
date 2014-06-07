#pragma once

#include <GL/glew.h>

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
class IndexedVertexBuffer
{
public:
    typedef std::vector<ArbitraryMeshVertex> Vertices;
    typedef std::vector<RenderIndex> Indices;

private:

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

    /// Render all batches with given primitive type
    void renderAllBatches(GLenum primitiveType) const
    {
        if (_vertexVBO == 0 || _indexVBO == 0)
        {
            initialiseVBOs();
        }

        glBindBuffer(GL_ARRAY_BUFFER, _vertexVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexVBO);

        // Vertex pointer includes whole vertex buffer
        glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex),
                        ArbitraryMeshVertex::VERTEX_OFFSET());

        // Render each batch of indices
        for (std::vector<Batch>::const_iterator i = _batches.begin();
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
