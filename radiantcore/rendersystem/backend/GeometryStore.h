#pragma once

#include <stdexcept>
#include <vector>
#include <limits>
#include "render/ArbitraryMeshVertex.h"
#include "render/ContinuousBuffer.h"

namespace render
{

/**
 * Storage container for indexed vertex data.
 * 
 * Client code will allocate fixed-size blocks of continuous
 * memory for vertex and index data.
 * 
 * The Block handle will remain valid until relasing it,
 * though the underlying memory location is subject to change.
 * 
 * Blocks cannot be resized after allocation.
 * 
 * All the vertex data will is guaranteed to belong to the same continuous
 * large block of memory, making it suitable for openGL multi draw calls.
 */
class GeometryStore
{
public:
    // Slot ID handed out to client code
    using Slot = std::uint64_t;

private:
    ContinuousBuffer<ArbitraryMeshVertex> _vertexBuffer;
    ContinuousBuffer<unsigned int> _indexBuffer;

public:
    GeometryStore()
    {}

    Slot allocateSlot(const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices)
    {
        assert(!vertices.empty());
        assert(!indices.empty());

        auto vertexSlot = _vertexBuffer.allocate(vertices.size());
        _vertexBuffer.setData(vertexSlot, vertices);

        auto indexSlot = _indexBuffer.allocate(indices.size());
        _indexBuffer.setData(indexSlot, indices);

        return GetSlot(vertexSlot, indexSlot);
    }

    void deallocateSlot(Slot slot)
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
