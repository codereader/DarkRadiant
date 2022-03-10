#pragma once

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

    public:
        ~BufferObject()
        {

        }

        void setData(std::size_t offset, const unsigned char* firstElement, std::size_t numBytes) override
        {

        }

        // Re-allocates the memory of this buffer, does not transfer the data 
        // from the old internal buffer to the new one.
        void resize(std::size_t newSize) override
        {

        }
    };

public:
    IBufferObject::Ptr createBufferObject() override
    {
        return std::make_shared<BufferObject>();
    }
};

}
