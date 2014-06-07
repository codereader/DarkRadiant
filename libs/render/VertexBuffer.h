#pragma once

#include <GL/glew.h>

#include "ArbitraryMeshVertex.h"
#include "VBO.h"

namespace render
{

/**
 * \brief
 * Receiver of vertex geometry for rendering
 *
 * A VertexBuffer provides methods to accept batches of vertices for rendering.
 * Vertices can be submitted in several batches, and each batch can be rendered
 * separately (and more than once) without recreating the vertex buffer. The
 * VertexBuffer may make use of an OpenGL VBO for improved performance.
 */
class VertexBuffer
{
public:
    typedef std::vector<ArbitraryMeshVertex> Vertices;

private:

    // OpenGL VBO information
    mutable GLuint _vboID;

    // Initial non-VBO based vertex storage
    Vertices _vertices;

    // Batch start index and size
    struct Batch
    {
        std::size_t start;
        std::size_t size;
    };

    // All batches
    std::vector<Batch> _batches;

private:

    // Create the VBO and copy all vertex data into it
    void initialiseVBO() const
    {
        _vboID = makeVBOFromArray(GL_ARRAY_BUFFER, _vertices);

        if (_vboID == 0)
        {
            std::runtime_error("Could not create vertex buffer");
        }
    }

public:

    /// Default construct with no initial resource allocation
    VertexBuffer()
    : _vboID(0)
    { }

    /// Destroy all resources
    ~VertexBuffer()
    {
        deleteVBO(_vboID);
    }

    /**
     * \brief
     * Add a batch of vertices
     *
     * \param begin
     * Iterator pointing to the first ArbitraryMeshVertex in the batch.
     *
     * \param count
     * Number of vertices in the batch.
     *
     * \param stride
     * How much to increment the input iterator between each vertex (defaults
     * to 1).
     */
    template<typename Iter_T>
    void addBatch(Iter_T begin, std::size_t count, std::size_t stride = 1)
    {
        if (count < 1)
        {
            throw std::logic_error("Batch must contain at least one vertex");
        }

        // Store batch information
        Batch newBatch = { _vertices.size(), count };
        _batches.push_back(newBatch);

        // Append all vertices
        _vertices.reserve(_vertices.size() + count);
        for (Iter_T i = begin; i < begin + (count * stride); i += stride)
        {
            _vertices.push_back(*i);
        }
    }

    /**
     * \brief
     * Replace data with that from another VertexBuffer
     *
     * If the other VertexBuffer is the same size or smaller than this one and
     * has not yet had its own VBO allocated, this may improve performance by
     * avoiding unnecessary re-allocations of GPU memory.
     *
     * This method may call GL functions so requires a valid GL context.
     */
    void replaceData(const VertexBuffer& other)
    {
        replaceVBODataIfPossible(GL_ARRAY_BUFFER, _vboID,
                                 _vertices, other._vertices);

        _vertices = other._vertices;
        _batches = other._batches;
    }

    /// Render all batches with the given primitive type
    void renderAllBatches(GLenum primitiveType) const
    {
        if (_vboID == 0)
        {
            initialiseVBO();
        }
        glBindBuffer(GL_ARRAY_BUFFER, _vboID);

        // Vertex pointer is always at the start of the whole buffer (the start
        // and count parameters to glDrawArrays separate batches).
        glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex),
                        ArbitraryMeshVertex::VERTEX_OFFSET());

        // For each batch
        for (std::vector<Batch>::const_iterator i = _batches.begin();
             i != _batches.end();
             ++i)
        {
            glDrawArrays(primitiveType, static_cast<GLint>(i->start),
                         static_cast<GLsizei>(i->size));
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

}
