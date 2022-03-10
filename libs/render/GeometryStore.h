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
    enum class SlotType
    {
        Regular = 0,
        IndexRemap = 1,
    };

    static constexpr auto NumFrameBuffers = 2;

    // Keep track of modified slots as long as a single buffer is in use
    std::vector<detail::BufferTransaction> _transactionLog;

    // Represents the storage for a single frame
    struct FrameBuffer
    {
        ContinuousBuffer<MeshVertex> vertices;
        ContinuousBuffer<unsigned int> indices;

        ISyncObject::Ptr syncObject;

        IBufferObject::Ptr vertexBufferObject;
        IBufferObject::Ptr indexBufferObject;

        void applyTransactions(const std::vector<detail::BufferTransaction>& transactions, const FrameBuffer& other)
        {
            vertices.applyTransactions(transactions, other.vertices, GetVertexSlot);
            indices.applyTransactions(transactions, other.indices, GetIndexSlot);
        }

        void syncToBufferObjects()
        {
            vertices.syncModificationsToBufferObject(vertexBufferObject);
            indices.syncModificationsToBufferObject(indexBufferObject);
        }
    };

    // We keep a fixed number of frame buffers
    std::vector<FrameBuffer> _frameBuffers;
    unsigned int _currentBuffer;

    ISyncObjectProvider& _syncObjectProvider;

public:
    GeometryStore(ISyncObjectProvider& syncObjectProvider, IBufferObjectProvider& bufferObjectProvider) :
        _currentBuffer(0),
        _syncObjectProvider(syncObjectProvider)
    {
        _frameBuffers.resize(NumFrameBuffers);

        // Assign (empty) buffer objects to the frames
        for (auto& frameBuffer : _frameBuffers)
        {
            frameBuffer.vertexBufferObject = bufferObjectProvider.createBufferObject();
            frameBuffer.indexBufferObject = bufferObjectProvider.createBufferObject();
        }
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

    void syncToBufferObjects()
    {
        auto& current = getCurrentBuffer();
        current.syncToBufferObjects();
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

        auto slot = GetSlot(SlotType::Regular, vertexSlot, indexSlot);

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Allocate
        });

        return slot;
    }

    Slot allocateIndexSlot(Slot slotContainingVertexData, std::size_t numIndices) override
    {
        assert(numIndices > 0);

        auto& current = getCurrentBuffer();

        // Check the primary slot, it must be one containing vertex data
        if (GetSlotType(slotContainingVertexData) != SlotType::Regular)
        {
            throw std::logic_error("The given slot doesn't contain any vertex data and cannot be used as index remap base");
        }

        auto indexSlot = current.indices.allocate(numIndices);

        // In an IndexRemap slot, the vertex slot ID refers to the one containing the vertices
        auto slot = GetSlot(SlotType::IndexRemap, GetVertexSlot(slotContainingVertexData), indexSlot);

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Allocate
        });

        return slot;
    }

    void updateData(Slot slot, const std::vector<MeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        auto& current = getCurrentBuffer();

        if (GetSlotType(slot) == SlotType::Regular)
        {
            assert(!vertices.empty());
            current.vertices.setData(GetVertexSlot(slot), vertices);
        }
        else if (!vertices.empty()) // index slots cannot resize vertex data
        {
            throw std::logic_error("This is an index remap slot, cannot update vertex data");
        }
         
        assert(!indices.empty());
        current.indices.setData(GetIndexSlot(slot), indices);

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Update
        });
    }

    void updateSubData(Slot slot, std::size_t vertexOffset, const std::vector<MeshVertex>& vertices,
        std::size_t indexOffset, const std::vector<unsigned int>& indices) override
    {
        auto& current = getCurrentBuffer();

        if (GetSlotType(slot) == SlotType::Regular)
        {
            assert(!vertices.empty());
            current.vertices.setSubData(GetVertexSlot(slot), vertexOffset, vertices);
        }
        else if (!vertices.empty()) // index slots cannot resize vertex data
        {
            throw std::logic_error("This is an index remap slot, cannot update vertex data");
        }

        assert(!indices.empty());
        current.indices.setSubData(GetIndexSlot(slot), indexOffset, indices);

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Update
        });
    }

    void resizeData(Slot slot, std::size_t vertexSize, std::size_t indexSize) override
    {
        auto& current = getCurrentBuffer();

        if (GetSlotType(slot) == SlotType::Regular)
        {
            current.vertices.resizeData(GetVertexSlot(slot), vertexSize);
        }
        else if (vertexSize > 0)
        {
            throw std::logic_error("This is an index remap slot, cannot resize vertex data");
        }

        current.indices.resizeData(GetIndexSlot(slot), indexSize);

        _transactionLog.emplace_back(detail::BufferTransaction{
            slot, detail::BufferTransaction::Type::Update
        });
    }

    void deallocateSlot(Slot slot) override
    {
        auto& current = getCurrentBuffer();

        // Release the vertex data only for regular slot
        // IndexRemap slots leave the referenced primary slot alone
        if (GetSlotType(slot) == SlotType::Regular)
        {
            current.vertices.deallocate(GetVertexSlot(slot));
        }

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
        auto& current = getCurrentBuffer();

        // Acquire the slot containing the vertices
        auto vertexSlot = GetVertexSlot(slot);
        auto vertex = current.vertices.getBufferStart() + current.vertices.getOffset(vertexSlot);

        // Get the indices and use them to iterate over the vertices
        auto indexSlot = GetIndexSlot(slot); 
        auto indexPointer = current.indices.getBufferStart() + current.indices.getOffset(indexSlot);
        auto numIndices = current.indices.getNumUsedElements(indexSlot);

        AABB bounds;

        for (auto i = 0; i < numIndices; ++i, ++indexPointer)
        {
            bounds.includePoint(vertex[*indexPointer].vertex);
        }

        return bounds;
    }

private:
    FrameBuffer& getCurrentBuffer()
    {
        return _frameBuffers[_currentBuffer];
    }

    // Highest 2 bits define the type, then 2x 31 bits are used for the vertex and index slot IDs
    static Slot GetSlot(SlotType slotType, std::uint32_t vertexSlot, std::uint32_t indexSlot)
    {
        // Remove the highest bit from vertex and index slot numbers, then assign the highest two
        return (static_cast<Slot>(vertexSlot & 0x7FFFFFFF) << 31) | 
               (static_cast<Slot>(indexSlot & 0x7FFFFFFF)) |
               (static_cast<Slot>(slotType) << 62);
    }

    static SlotType GetSlotType(Slot slot)
    {
        return static_cast<SlotType>(slot >> 62);
    }

    static std::uint32_t GetVertexSlot(Slot slot)
    {
        return static_cast<std::uint32_t>(slot >> 31) & 0x7FFFFFFF; // Clear the highest bit
    }

    static std::uint32_t GetIndexSlot(Slot slot)
    {
        return static_cast<std::uint32_t>(slot) & 0x7FFFFFFF; // Clear the highest bit
    }
};

}
