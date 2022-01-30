#pragma once

#include <stdexcept>
#include <limits>
#include "igeometrystore.h"
#include "render/ContinuousBuffer.h"

namespace render
{

class GeometryStore :
    public IGeometryStore
{
public:
    // Slot ID handed out to client code
    using Slot = std::uint64_t;

private:
    ContinuousBuffer<ArbitraryMeshVertex> _vertexBuffer;
    ContinuousBuffer<unsigned int> _indexBuffer;

public:
    Slot allocateSlot(const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        assert(!vertices.empty());
        assert(!indices.empty());

        auto vertexSlot = _vertexBuffer.allocate(vertices.size());
        _vertexBuffer.setData(vertexSlot, vertices);

        auto indexSlot = _indexBuffer.allocate(indices.size());
        _indexBuffer.setData(indexSlot, indices);

        return GetSlot(vertexSlot, indexSlot);
    }

    void deallocateSlot(Slot slot) override
    {
        _vertexBuffer.deallocate(GetVertexSlot(slot));
        _indexBuffer.deallocate(GetIndexSlot(slot));
    }

private:
    // Higher 4 bytes will hold the vertex buffer slot
    static Slot GetSlot(std::uint32_t vertexSlot, std::uint32_t indexSlot)
    {
        return (static_cast<Slot>(vertexSlot) << 32) + indexSlot;
    }

    static std::uint32_t GetVertexSlot(Slot slot)
    {
        return static_cast<std::uint32_t>(slot >> 32);
    }

    static std::uint32_t GetIndexSlot(Slot slot)
    {
        return static_cast<std::uint32_t>((slot << 32) >> 32);
    }
};

}
