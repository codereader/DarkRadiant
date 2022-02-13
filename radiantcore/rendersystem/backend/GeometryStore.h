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
    static constexpr auto NumFrameBuffers = 2;

    // Keep track of modified slots as long as a single buffer is in use
    std::vector<detail::BufferTransaction> _transactionLog;

    // Represents the storage for a single frame
    struct FrameBuffer
    {
        ContinuousBuffer<ArbitraryMeshVertex> vertices;
        ContinuousBuffer<unsigned int> indices;

        GLsync syncObject;

        FrameBuffer() :
            syncObject(nullptr)
        {}

        void applyTransactions(const std::vector<detail::BufferTransaction>& transactions, const FrameBuffer& other)
        {
            vertices.applyTransactions(transactions, other.vertices, GetVertexSlot);
            indices.applyTransactions(transactions, other.indices, GetIndexSlot);
        }
    };

    // We keep a fixed number of frame buffers
    std::vector<FrameBuffer> _frameBuffers;
    unsigned int _currentBuffer;

public:
    GeometryStore() :
        _currentBuffer(0)
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
        if (current.syncObject != nullptr)
        {
            auto result = glClientWaitSync(current.syncObject, 0, GL_TIMEOUT_IGNORED);

            while (result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED)
            {
                result = glClientWaitSync(current.syncObject, 0, GL_TIMEOUT_IGNORED);

                if (result == GL_WAIT_FAILED)
                {
                    throw std::runtime_error("Could not acquire frame buffer lock");
                }
            }

            glDeleteSync(current.syncObject);
            current.syncObject = nullptr;
        }

        // Replay any modifications to the new buffer
        current.applyTransactions(_transactionLog, previous);
        _transactionLog.clear();
    }

    // Completes the currently writing frame, creates sync objects
    void onFrameFinished()
    {
        auto& current = getCurrentBuffer();
        current.syncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    Slot allocateSlot(const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        assert(!vertices.empty());
        assert(!indices.empty());

        auto& current = getCurrentBuffer();

        auto vertexSlot = current.vertices.allocate(vertices.size());
        current.vertices.setData(vertexSlot, vertices);

        auto indexSlot = current.indices.allocate(indices.size());
        current.indices.setData(indexSlot, indices);

        auto slot = GetSlot(vertexSlot, indexSlot);

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Allocate
        });

        return slot;
    }

    void updateData(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices,
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
            current.indices.getSize(indexSlot), // index count of the given geometry
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
