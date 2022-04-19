#pragma once

#include <stdexcept>
#include "igl.h"
#include "igeometrystore.h"

namespace render
{

class BufferObjectProvider final :
    public IBufferObjectProvider
{
private:
    class BufferObject final : 
        public IBufferObject
    {
    private:
        IBufferObject::Type _type;
        GLuint _buffer;
        GLenum _target;

    public:
        BufferObject(IBufferObject::Type type) :
            _type(type),
            _buffer(0),
            _target(_type == Type::Vertex ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER)
        {}

        ~BufferObject()
        {
            if (_buffer != 0)
            {
                glDeleteBuffers(1, &_buffer);
            }
            
            _buffer = 0;
        }

        void bind() override
        {
            glBindBuffer(_target, _buffer);
        }

        void unbind() override
        {
            glBindBuffer(_target, 0);
        }

        void setData(std::size_t offset, const unsigned char* firstElement, std::size_t numBytes) override
        {
            glBufferSubData(_target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(numBytes), firstElement);
        }

        // Re-allocates the memory of this buffer, does not transfer the data 
        // from the old internal buffer to the new one.
        void resize(std::size_t newSize) override
        {
            if (_buffer == 0)
            {
                glGenBuffers(1, &_buffer);
            }

            glBindBuffer(_target, _buffer);

#ifndef NDEBUG
            if (!glIsBuffer(_buffer))
            {
                throw std::runtime_error("Failed to generate a GL buffer object");
            }
#endif

            glBufferData(_target, static_cast<GLsizeiptr>(newSize), nullptr, GL_DYNAMIC_DRAW);

            glBindBuffer(_target, 0);
        }
    };

public:
    IBufferObject::Ptr createBufferObject(IBufferObject::Type type) override
    {
        return std::make_shared<BufferObject>(type);
    }
};

}
