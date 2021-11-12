#pragma once

#include "isurfacerenderer.h"

namespace render
{

class SurfaceRenderer :
    public ISurfaceRenderer
{
private:
    std::vector<ArbitraryMeshVertex> _vertices;
    std::vector<unsigned int> _indices;

    static constexpr std::size_t InvalidVertexIndex = std::numeric_limits<std::size_t>::max();

    struct SlotInfo
    {
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
    {}

    bool empty() const
    {
        return _vertices.empty();
    }

    Slot addSurface(const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        // Allocate a slot
        auto oldVertexSize = _vertices.size();
        auto oldIndexSize = _indices.size();

        auto newSlotIndex = getNextFreeSlotMapping();

        auto& slot = _slots.at(newSlotIndex);
        
        slot.firstVertex = oldVertexSize;
        slot.numVertices = vertices.size();
        slot.firstIndex = oldIndexSize;
        slot.numIndices = indices.size();

        _vertices.reserve(oldVertexSize + vertices.size()); // reserve() never shrinks
        std::copy(vertices.begin(), vertices.end(), std::back_inserter(_vertices));

        // Allocate, copy and offset indices
        _indices.reserve(oldIndexSize + indices.size());

        for (auto index : indices)
        {
            _indices.push_back(index + static_cast<unsigned int>(oldVertexSize));
        }

        return newSlotIndex;
    }

    void removeSurface(Slot slot) override
    {
        auto& slotInfo = _slots.at(slot);

        // Cut out the vertices
        auto firstVertexToRemove = _vertices.begin() + slotInfo.firstVertex;
        _vertices.erase(firstVertexToRemove, firstVertexToRemove + slotInfo.numVertices);

        // Shift all indices to the left, offsetting their values by the number of removed vertices
        auto offsetToApply = -static_cast<int>(slotInfo.numVertices);

        auto targetIndex = _indices.begin() + slotInfo.firstIndex;
        auto indexToMove = targetIndex + slotInfo.numIndices;

        auto indexEnd = _indices.end();
        while (indexToMove != indexEnd)
        {
            *targetIndex++ = *indexToMove++ + offsetToApply;
        }

        // Cut off the tail of the indices
        _indices.resize(_indices.size() - slotInfo.numIndices);

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

        // Copy the data to the correct slot in the array
        std::copy(vertices.begin(), vertices.end(), _vertices.begin() + slotInfo.firstVertex);

        // Before assignment, the indices need to be shifted to match the array offset of the vertices
        auto targetIndex = _indices.begin() + slotInfo.firstIndex;
        auto indexShift = static_cast<unsigned int>(slotInfo.firstVertex);

        for (auto index : indices)
        {
            *targetIndex++ = index + indexShift;
        }
    }

    void render()
    {
        // Vertex pointer includes whole vertex buffer
        glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().vertex);
        glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().texcoord);
        glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().normal);
            
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, &_indices.front());
    }

private:
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