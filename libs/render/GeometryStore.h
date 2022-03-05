#pragma once

#include <stdexcept>
#include <limits>
#include "igeometrystore.h"
#include "ContinuousBuffer.h"

namespace render
{

class GeometryStore :
    public IGeometryStore
{
public:
    // Slot ID handed out to client code
    using Slot = std::uint64_t;

private:
    static constexpr auto NumFrameBuffers = 2;

    // Keep track of modified slots as long as a single buffer is in use
    std::vector<detail::BufferTransaction> _transactionLog;

    // Represents the storage for a single frame
    struct FrameBuffer
    {
        ContinuousBuffer<MeshVertex> vertices;
        ContinuousBuffer<unsigned int> indices;

        ISyncObject::Ptr syncObject;

        void applyTransactions(const std::vector<detail::BufferTransaction>& transactions, const FrameBuffer& other)
        {
            vertices.applyTransactions(transactions, other.vertices, GetVertexSlot);
            indices.applyTransactions(transactions, other.indices, GetIndexSlot);
        }
    };

    // We keep a fixed number of frame buffers
    std::vector<FrameBuffer> _frameBuffers;
    unsigned int _currentBuffer;

    ISyncObjectProvider& _syncObjectProvider;

public:
    GeometryStore(ISyncObjectProvider& syncObjectProvider) :
        _currentBuffer(0),
        _syncObjectProvider(syncObjectProvider)
    {
        _frameBuffers.resize(NumFrameBuffers);
    }

    // Marks the beginning of a frame, switches to the next writing buffers 
    void onFrameStart()
    {
        // Switch to the next frame
        auto& previous = getCurrentBuffer();

        _currentBuffer = (_currentBuffer + 1) % NumFrameBuffers;
        auto& current = getCurrentBuffer();

        // Wait for this buffer to become available
        if (current.syncObject)
        {
            current.syncObject->wait();
            current.syncObject.reset();
        }

        // Replay any modifications to the new buffer
        current.applyTransactions(_transactionLog, previous);
        _transactionLog.clear();
    }

    // Completes the currently writing frame, creates sync objects
    void onFrameFinished()
    {
        auto& current = getCurrentBuffer();
        current.syncObject = _syncObjectProvider.createSyncObject();
    }

    Slot allocateSlot(std::size_t numVertices, std::size_t numIndices) override
    {
        assert(numVertices > 0);
        assert(numIndices > 0);

        auto& current = getCurrentBuffer();

        auto vertexSlot = current.vertices.allocate(numVertices);
        auto indexSlot = current.indices.allocate(numIndices);

        auto slot = GetSlot(vertexSlot, indexSlot);

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Allocate
        });

        return slot;
    }

    void updateData(Slot slot, const std::vector<MeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        assert(!vertices.empty());
        assert(!indices.empty());

        auto& current = getCurrentBuffer();
        current.vertices.setData(GetVertexSlot(slot), vertices);
        current.indices.setData(GetIndexSlot(slot), indices);

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Update
        });
    }

    void updateSubData(Slot slot, std::size_t vertexOffset, const std::vector<MeshVertex>& vertices,
        std::size_t indexOffset, const std::vector<unsigned int>& indices) override
    {
        assert(!vertices.empty());
        assert(!indices.empty());

        auto& current = getCurrentBuffer();

        current.vertices.setSubData(GetVertexSlot(slot), vertexOffset, vertices);
        current.indices.setSubData(GetIndexSlot(slot), indexOffset, indices);

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Update
        });
    }

    void resizeData(Slot slot, std::size_t vertexSize, std::size_t indexSize) override
    {
        auto& current = getCurrentBuffer();

        current.vertices.resizeData(GetVertexSlot(slot), vertexSize);
        current.indices.resizeData(GetIndexSlot(slot), indexSize);

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Update
        });
    }

    void deallocateSlot(Slot slot) override
    {
        auto& current = getCurrentBuffer();
        current.vertices.deallocate(GetVertexSlot(slot));
        current.indices.deallocate(GetIndexSlot(slot));

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Deallocate
        });
    }

    RenderParameters getRenderParameters(Slot slot) override
    {
        auto vertexSlot = GetVertexSlot(slot);
        auto indexSlot = GetIndexSlot(slot);

        auto& current = getCurrentBuffer();

        return RenderParameters
        {
            current.vertices.getBufferStart(),
            current.indices.getBufferStart() + current.indices.getOffset(indexSlot), // pointer to first index
            current.indices.getNumUsedElements(indexSlot), // index count of the given geometry
            current.vertices.getOffset(vertexSlot) // offset to the first vertex
        };
    }

    AABB getBounds(Slot slot) override
    {
        auto vertexSlot = GetVertexSlot(slot);
        auto& current = getCurrentBuffer();

        auto vertex = current.vertices.getBufferStart() + current.vertices.getOffset(vertexSlot);
        auto numVertices = current.vertices.getSize(vertexSlot);

        AABB bounds;

        for (auto i = 0; i < numVertices; ++i, ++vertex)
        {
            bounds.includePoint(vertex->vertex);
        }

        return bounds;
    }

private:
    FrameBuffer& getCurrentBuffer()
    {
        return _frameBuffers[_currentBuffer];
    }

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
