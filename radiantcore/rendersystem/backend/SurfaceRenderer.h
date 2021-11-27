#pragma once

#include "isurfacerenderer.h"

namespace render
{

class SurfaceRenderer :
    public ISurfaceRenderer
{
private:
    struct VertexBuffer
    {
        GLenum mode;
        std::vector<ArbitraryMeshVertex> vertices;
        std::vector<unsigned int> indices;
    };

    VertexBuffer _triangleBuffer;
    VertexBuffer _quadBuffer;

    static constexpr std::size_t InvalidVertexIndex = std::numeric_limits<std::size_t>::max();

    struct SlotInfo
    {
        std::uint8_t bucketIndex;
        std::size_t firstVertex;
        std::size_t numVertices;
        std::size_t firstIndex;
        std::size_t numIndices;
    };
    std::vector<SlotInfo> _slots;

    static constexpr std::size_t InvalidSlotMapping = std::numeric_limits<std::size_t>::max();
    std::size_t _freeSlotMappingHint;

public:
    SurfaceRenderer() :
        _freeSlotMappingHint(InvalidSlotMapping)
    {
        _triangleBuffer.mode = GL_TRIANGLES;
        _quadBuffer.mode = GL_QUADS;
    }

    bool empty() const
    {
        return _triangleBuffer.vertices.empty() && _quadBuffer.vertices.empty();
    }

    Slot addSurface(SurfaceIndexingType indexType, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        auto bucketIndex = GetBucketIndexForIndexType(indexType);
        auto& bucket = getBucketByIndex(bucketIndex);

        // Allocate a slot
        auto oldVertexSize = bucket.vertices.size();
        auto oldIndexSize = bucket.indices.size();

        auto newSlotIndex = getNextFreeSlotMapping();

        auto& slot = _slots.at(newSlotIndex);
        
        slot.bucketIndex = bucketIndex;
        slot.firstVertex = oldVertexSize;
        slot.numVertices = vertices.size();
        slot.firstIndex = oldIndexSize;
        slot.numIndices = indices.size();

        std::copy(vertices.begin(), vertices.end(), std::back_inserter(bucket.vertices));

        for (auto index : indices)
        {
            bucket.indices.push_back(index + static_cast<unsigned int>(oldVertexSize));
        }

        return newSlotIndex;
    }

    void removeSurface(Slot slot) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& bucket = getBucketByIndex(slotInfo.bucketIndex);

        // Cut out the vertices
        auto firstVertexToRemove = bucket.vertices.begin() + slotInfo.firstVertex;
        bucket.vertices.erase(firstVertexToRemove, firstVertexToRemove + slotInfo.numVertices);

        // Shift all indices to the left, offsetting their values by the number of removed vertices
        auto offsetToApply = -static_cast<int>(slotInfo.numVertices);

        auto targetIndex = bucket.indices.begin() + slotInfo.firstIndex;
        auto indexToMove = targetIndex + slotInfo.numIndices;

        auto indexEnd = bucket.indices.end();
        while (indexToMove != indexEnd)
        {
            *targetIndex++ = *indexToMove++ + offsetToApply;
        }

        // Cut off the tail of the indices
        bucket.indices.resize(bucket.indices.size() - slotInfo.numIndices);

        // Adjust all offsets in other slots
        for (auto& slot : _slots)
        {
            if (slot.firstVertex > slotInfo.firstVertex)
            {
                slot.firstVertex -= slotInfo.numVertices;
                slot.firstIndex -= slotInfo.numIndices;
            }
        }

        // Invalidate the slot
        slotInfo.numVertices = 0;
        slotInfo.firstVertex = InvalidVertexIndex;
        slotInfo.firstIndex = 0;
        slotInfo.numIndices = 0;
        
        if (slot < _freeSlotMappingHint)
        {
            _freeSlotMappingHint = slot;
        }
    }

    void updateSurface(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& bucket = getBucketByIndex(slotInfo.bucketIndex);

        // Copy the data to the correct slot in the array
        std::copy(vertices.begin(), vertices.end(), bucket.vertices.begin() + slotInfo.firstVertex);

        // Before assignment, the indices need to be shifted to match the array offset of the vertices
        auto targetIndex = bucket.indices.begin() + slotInfo.firstIndex;
        auto indexShift = static_cast<unsigned int>(slotInfo.firstVertex);

        for (auto index : indices)
        {
            *targetIndex++ = index + indexShift;
        }
    }

    void render()
    {
        renderBuffer(_triangleBuffer);
        renderBuffer(_quadBuffer);
    }

    void renderSurface(Slot slot) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& buffer = getBucketByIndex(slotInfo.bucketIndex);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

        glFrontFace(GL_CW);

        glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &buffer.vertices.front().vertex);
        glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &buffer.vertices.front().texcoord);
        glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &buffer.vertices.front().normal);

        glDrawElements(buffer.mode, static_cast<GLsizei>(slotInfo.numIndices), GL_UNSIGNED_INT, &buffer.indices[slotInfo.firstIndex]);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

private:

    void renderBuffer(const VertexBuffer& buffer)
    {
        if (!buffer.indices.empty())
        {
            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &buffer.vertices.front().vertex);
            glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &buffer.vertices.front().texcoord);
            glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &buffer.vertices.front().normal);

            glDrawElements(buffer.mode, static_cast<GLsizei>(buffer.indices.size()), GL_UNSIGNED_INT, &buffer.indices.front());
        }
    }

    constexpr static std::uint8_t GetBucketIndexForIndexType(SurfaceIndexingType indexType)
    {
        return indexType == SurfaceIndexingType::Triangles ? 0 : 1;
    }

    VertexBuffer& getBucketByIndex(std::uint8_t bucketIndex)
    {
        return bucketIndex == 0 ? _triangleBuffer : _quadBuffer;
    }

    ISurfaceRenderer::Slot getNextFreeSlotMapping()
    {
        auto numSlots = _slots.size();

        for (auto i = _freeSlotMappingHint; i < numSlots; ++i)
        {
            if (_slots[i].firstVertex == InvalidVertexIndex)
            {
                _freeSlotMappingHint = i + 1; // start searching here next time
                return i;
            }
        }

        _slots.emplace_back();
        return numSlots; // == the size before we emplaced the new slot
    }
};

}