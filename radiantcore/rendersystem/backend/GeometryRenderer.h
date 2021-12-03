#pragma once

#include "igeometryrenderer.h"

namespace render
{

class GeometryRenderer :
    public IGeometryRenderer
{
private:
    class VertexBuffer
    {
    private:
        GLenum _mode;

        std::vector<ArbitraryMeshVertex> _vertices;
        std::vector<unsigned int> _indices;
    public:
        VertexBuffer(GLenum mode) :
            _mode(mode)
        {}

        bool empty() const
        {
            return _indices.empty();
        }

        void render() const
        {
            if (_indices.empty()) return;

            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().vertex);
            glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().texcoord);
            glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().normal);

            glDrawElements(_mode, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, &_indices.front());
        }

        void renderIndexRange(std::size_t firstIndex, std::size_t numIndices) const
        {
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_NORMAL_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);

            glFrontFace(GL_CW);

            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().vertex);
            glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().texcoord);
            glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().normal);

            glDrawElements(_mode, static_cast<GLsizei>(numIndices), GL_UNSIGNED_INT, &_indices[firstIndex]);
            glDisableClientState(GL_NORMAL_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }

        // Returns the vertex and index offsets in this buffer
        std::pair<std::size_t, std::size_t> addSurface(const std::vector<ArbitraryMeshVertex>& vertices,
            const std::vector<unsigned int>& indices)
        {
            auto vertexOffset = _vertices.size();
            auto indexOffset = _indices.size();

            std::copy(vertices.begin(), vertices.end(), std::back_inserter(_vertices));

            for (auto index : indices)
            {
                _indices.push_back(index + static_cast<unsigned int>(vertexOffset));
            }

            return { vertexOffset, indexOffset };
        }

        void updateSurface(std::size_t firstVertex, std::size_t firstIndex, 
            const std::vector<ArbitraryMeshVertex>& vertices,
            const std::vector<unsigned int>& indices)
        {
            // Copy the data to the correct slot in the array
            std::copy(vertices.begin(), vertices.end(), _vertices.begin() + firstVertex);

            // Before assignment, the indices need to be shifted to match the array offset of the vertices
            auto targetIndex = _indices.begin() + firstIndex;
            auto indexShift = static_cast<unsigned int>(firstVertex);

            for (auto index : indices)
            {
                *targetIndex++ = index + indexShift;
            }
        }

        // Cuts out the vertices and indices, adjusts all indices located to the right of the cut
        void removeSurface(std::size_t firstVertex, std::size_t numVertices, std::size_t firstIndex, std::size_t numIndices)
        {
            // Cut out the vertices
            auto firstVertexToRemove = _vertices.begin() + firstVertex;
            _vertices.erase(firstVertexToRemove, firstVertexToRemove + numVertices);

            // Shift all indices to the left, offsetting their values by the number of removed vertices
            auto offsetToApply = -static_cast<int>(numVertices);

            auto targetIndex = _indices.begin() + firstIndex;
            auto indexToMove = targetIndex + numIndices;

            auto indexEnd = _indices.end();
            while (indexToMove != indexEnd)
            {
                *targetIndex++ = *indexToMove++ + offsetToApply;
            }

            // Cut off the tail of the indices
            _indices.resize(_indices.size() - numIndices);
        }
    };

    std::vector<VertexBuffer> _buffers;

    static constexpr std::size_t InvalidVertexIndex = std::numeric_limits<std::size_t>::max();

    // Internal information about where the chunk of indexed vertex data is located:
    // Which buffer they're in, and the data offset and count within the buffer.
    // This is enough information to access, replace or remove the data at a later point.
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
    GeometryRenderer() :
        _freeSlotMappingHint(InvalidSlotMapping)
    {
        _buffers.emplace_back(GL_TRIANGLES);
        _buffers.emplace_back(GL_QUADS);
    }

    bool empty() const
    {
        for (const auto& buffer : _buffers)
        {
            if (!buffer.empty()) return false;
        }

        return true;
    }

    Slot addSurface(SurfaceIndexingType indexType, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        auto bucketIndex = GetBucketIndexForIndexType(indexType);
        auto& bucket = getBucketByIndex(bucketIndex);

        // Allocate a slot
        auto newSlotIndex = getNextFreeSlotMapping();
        auto& slot = _slots.at(newSlotIndex);

        auto [vertexOffset, indexOffset] = bucket.addSurface(vertices, indices);

        slot.bucketIndex = bucketIndex;
        slot.firstVertex = vertexOffset;
        slot.numVertices = vertices.size();
        slot.firstIndex = indexOffset;
        slot.numIndices = indices.size();

        return newSlotIndex;
    }

    void removeSurface(Slot slot) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& bucket = getBucketByIndex(slotInfo.bucketIndex);

        bucket.removeSurface(slotInfo.firstVertex, slotInfo.numVertices, slotInfo.firstIndex, slotInfo.numIndices);

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

        bucket.updateSurface(slotInfo.firstVertex, slotInfo.firstIndex, vertices, indices);
    }

    void render()
    {
        for (auto& buffer : _buffers)
        {
            buffer.render();
        }
    }

    void renderSurface(Slot slot) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& buffer = getBucketByIndex(slotInfo.bucketIndex);

        buffer.renderIndexRange(slotInfo.firstIndex, slotInfo.numIndices);
    }

private:
    constexpr static std::uint8_t GetBucketIndexForIndexType(SurfaceIndexingType indexType)
    {
        return indexType == SurfaceIndexingType::Triangles ? 0 : 1;
    }

    VertexBuffer& getBucketByIndex(std::uint8_t bucketIndex)
    {
        return _buffers[bucketIndex];
    }

    Slot getNextFreeSlotMapping()
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