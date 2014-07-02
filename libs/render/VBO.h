/**
 * \file
 * Utility functions for manipulating VBOs.
 */
#pragma once

#include <GL/glew.h>

namespace render
{

namespace detail
{
    template<typename Array_T>
    GLsizei byteSize(const Array_T& array)
    {
        return GLsizei(array.size() * sizeof(typename Array_T::value_type));
    }
}

/**
 * \brief
 * Generate a VBO from the given data
 *
 * \param target
 * Binding point for the VBO (e.g. GL_ARRAY_BUFFER).
 *
 * \param data
 * Array of data to copy into the VBO.
 */
template<typename Array_T>
GLuint makeVBOFromArray(GLenum target, const Array_T& data)
{
    // Create and bind
    GLuint vboID = 0;
    glGenBuffers(1, &vboID);
    glBindBuffer(target, vboID);

    // Copy data
    glBufferData(target, detail::byteSize(data), &data.front(), GL_STATIC_DRAW);

    // Return the VBO identifier
    return vboID;
}

/// Replace VBO data store with given array
template<typename Array_T>
void replaceVBOData(GLenum target, GLuint vboID, const Array_T& data)
{
    glBindBuffer(target, vboID);
    glBufferSubData(target, 0, detail::byteSize(data), &data.front());
}

/// Delete a VBO and set its identifier to 0
inline void deleteVBO(GLuint& id)
{
    if (id == 0) return;

    glDeleteBuffers(1, &id);
    id = 0;
}

/// Replace VBO data if new data is smaller than or equal to existing data
/**
 * \param target
 * VBO target (e.g. GL_ARRAY_BUFFER).
 *
 * \param vboID
 * Identifier variable for the VBO. If this is 0 (no VBO allocated), this
 * function does nothing. If the new data size is larger than the existing
 * data, this VBO is deleted and its identifier reset to 0.
 */
template<typename Array_T>
void replaceVBODataIfPossible(GLenum target, GLuint& vboID,
                              const Array_T& existingData,
                              const Array_T& newData)
{
    if (vboID != 0)
    {
        if (newData.size() <= existingData.size())
        {
            // Replace VBO data
            replaceVBOData(GL_ARRAY_BUFFER, vboID, newData);
        }
        else
        {
            // Size mismatch, cannot replace data so invalidate VBO
            deleteVBO(vboID);
        }
    }
}

}
