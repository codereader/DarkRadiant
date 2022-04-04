#pragma once

#include "igeometrystore.h"

namespace test
{

class TestBufferObject final :
    public render::IBufferObject
{
public:
    std::vector<unsigned char> buffer;

    std::size_t lastUsedOffset;
    std::size_t lastUsedByteCount;

    void bind() override {}
    void unbind() override {}

    void setData(std::size_t offset, const unsigned char* firstElement, std::size_t numBytes) override
    {
        lastUsedOffset = offset;
        lastUsedByteCount = numBytes;

        auto current = firstElement;
        for (auto i = 0; i < numBytes; ++i)
        {
            buffer.at(offset + i) = *current++;
        }
    }

    void resize(std::size_t newSize) override
    {
        buffer = std::vector<unsigned char>(newSize, '\0');
    }
};

class TestBufferObjectProvider final :
    public render::IBufferObjectProvider
{
public:
    render::IBufferObject::Ptr lastAllocatedVertexBuffer;
    render::IBufferObject::Ptr lastAllocatedIndexBuffer;

    render::IBufferObject::Ptr createBufferObject(render::IBufferObject::Type type) override
    {
        if (type == render::IBufferObject::Type::Vertex)
        {
            lastAllocatedVertexBuffer = std::make_shared<TestBufferObject>();
            return lastAllocatedVertexBuffer;
        }
        else
        {
            lastAllocatedIndexBuffer = std::make_shared<TestBufferObject>();
            return lastAllocatedIndexBuffer;
        }
    }
};

}
