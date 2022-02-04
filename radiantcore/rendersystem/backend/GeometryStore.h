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

    void updateData(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        assert(!vertices.empty());
        assert(!indices.empty());

        _vertexBuffer.setData(GetVertexSlot(slot), vertices);
        _indexBuffer.setData(GetIndexSlot(slot), indices);
    }

    void deallocateSlot(Slot slot) override
    {
        _vertexBuffer.deallocate(GetVertexSlot(slot));
        _indexBuffer.deallocate(GetIndexSlot(slot));
    }

    RenderParameters getRenderParameters(Slot slot) override
    {
        auto vertexSlot = GetVertexSlot(slot);
        auto indexSlot = GetIndexSlot(slot);

        return RenderParameters
        {
            _vertexBuffer.getBufferStart(),
            _indexBuffer.getBufferStart() + _indexBuffer.getOffset(indexSlot), // pointer to first index
            _indexBuffer.getSize(indexSlot), // index count of the given geometry
            _vertexBuffer.getOffset(vertexSlot) // offset to the first vertex
        };
    }

    AABB getBounds(Slot slot) override
    {
        auto vertexSlot = GetVertexSlot(slot);

        auto vertex = _vertexBuffer.getBufferStart() + _vertexBuffer.getOffset(vertexSlot);
        auto numVertices = _vertexBuffer.getSize(vertexSlot);

        AABB bounds;

        for (auto i = 0; i < numVertices; ++i, ++vertex)
        {
            bounds.includePoint(vertex->vertex);
        }

        return bounds;
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
