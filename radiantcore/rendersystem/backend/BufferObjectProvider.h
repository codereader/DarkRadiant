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
        std::size_t _allocatedSize;

    public:
        BufferObject(IBufferObject::Type type) :
            _type(type),
            _buffer(0),
            _target(_type == Type::Vertex ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER),
            _allocatedSize(0)
        {}

        ~BufferObject() override
        {
            if (_buffer != 0)
            {
                glDeleteBuffers(1, &_buffer);
            }

            _allocatedSize = 0;
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
            if (offset + numBytes > _allocatedSize)
            {
                throw std::runtime_error("Buffer is too small, resize first");
            }

            glBufferSubData(_target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(numBytes), firstElement);
            debug::assertNoGlErrors();
        }

        std::vector<unsigned char> getData(std::size_t offset, std::size_t numBytes) override
        {
            std::vector<unsigned char> data(numBytes, 255);

            glGetBufferSubData(_target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(numBytes), data.data());
            debug::assertNoGlErrors();

            return data;
        }

        // Re-allocates the memory of this buffer, does not transfer the data 
        // from the old internal buffer to the new one.
        void resize(std::size_t newSize) override
        {
            if (_buffer == 0)
            {
                glGenBuffers(1, &_buffer);
                debug::assertNoGlErrors();
            }

            glBindBuffer(_target, _buffer);

#ifndef NDEBUG
            if (!glIsBuffer(_buffer))
            {
                throw std::runtime_error("Failed to generate a GL buffer object");
            }
#endif

            glBufferData(_target, static_cast<GLsizeiptr>(newSize), nullptr, GL_DYNAMIC_DRAW);
            debug::assertNoGlErrors();

            _allocatedSize = newSize;

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
