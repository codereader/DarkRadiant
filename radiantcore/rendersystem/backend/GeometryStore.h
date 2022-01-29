#pragma once

#include <stdexcept>
#include <vector>
#include <limits>
#include "render/ArbitraryMeshVertex.h"

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
    template<typename ElementType>
    class Buffer
    {
    public:
        static constexpr std::size_t DefaultNumberOfElements = 65536;

    private:
        static constexpr std::size_t GrowthRate = 1; // 100% growth each time

        std::vector<ElementType> _buffer;

        struct SlotInfo
        {
            bool Occupied;      // whether this slot is free
            std::size_t Offset; // The index to the first element within the buffer
            std::size_t Size;   // Number of allocated elements

            SlotInfo(bool occupied, std::size_t offset, std::size_t size) :
                Occupied(occupied),
                Offset(offset),
                Size(size)
            {}
        };

        std::vector<SlotInfo> _slots;

    public:
        Buffer(std::size_t initialSize = DefaultNumberOfElements)
        {
            // Pre-allocate some memory, but don't go all the way down to zero
            _buffer.resize(initialSize == 0 ? 16 : initialSize);

            // The initial slot info which is going to be cut into pieces
            _slots.emplace_back(SlotInfo{
                false,
                0,
                _buffer.size()
            });
        }

        std::uint32_t allocate(std::size_t requiredSize)
        {
            return getNextFreeSlotForSize(requiredSize);
        }

        ElementType* getBufferStart()
        {
            return _buffer.data();
        }

        std::size_t getSize(std::uint32_t handle) const
        {
            return _slots[handle].Size;
        }

        std::size_t getOffset(std::uint32_t handle) const
        {
            return _slots[handle].Offset;
        }

        void setData(std::uint32_t handle, const std::vector<ElementType>& elements)
        {
            const auto& slot = _slots[handle];

            if (elements.size() != slot.Size)
            {
                throw std::logic_error("Allocation size mismatch in GeometryStore::Buffer::setData");
            }
            
            std::copy(elements.begin(), elements.end(), _buffer.begin() + slot.Offset);
        }

        void deallocate(std::uint32_t handle)
        {
            auto releasedSlot = _slots[handle];
            releasedSlot.Occupied = false;

            // Check if the slot can merge with an adjacent one
            if (handle > 0 && !_slots[handle - 1].Occupied)
            {
                // Merge the slot before the released one
                _slots[handle - 1].Size += releasedSlot.Size;

                // The released handle goes to waste, block it
                releasedSlot.Occupied = true;
                releasedSlot.Size = 0;
            }
            else if (handle < _slots.size() - 1 && !_slots[handle + 1].Occupied)
            {
                auto& nextSlot = _slots[handle + 1];

                releasedSlot.Size += nextSlot.Size;

                // The next slot will go to waste, never use that again
                nextSlot.Occupied = true;
                nextSlot.Size = 0;
            }
        }

    private:
        std::uint32_t getNextFreeSlotForSize(std::size_t requiredSize)
        {
            auto numSlots = _slots.size();
            std::uint32_t rightmostFreeSlotIndex = 0;
            std::size_t rightmostFreeOffset = 0;

            for (std::uint32_t slotIndex = 0; slotIndex < numSlots; ++slotIndex)
            {
                auto& slot = _slots[slotIndex];

                if (slot.Occupied) continue;

                // Keep track of the highest slot, we need that when re-allocating
                if (slot.Offset > rightmostFreeOffset)
                {
                    rightmostFreeOffset = slot.Offset;
                    rightmostFreeSlotIndex = slotIndex;
                }

                if (slot.Size < requiredSize) continue; // this slot is no use for us

                // Take it
                slot.Occupied = true;

                if (slot.Size > requiredSize)
                {
                    // Allocate a new free slot with the rest
                    _slots.emplace_back(SlotInfo{
                        false,
                        slot.Offset + requiredSize,
                        slot.Size - requiredSize,
                    });
                }

                return slotIndex;
            }

            // No space wherever, we need to expand the buffer
            auto additionalSize = std::max(_buffer.size() * GrowthRate, requiredSize);
            _buffer.resize(_buffer.size() + additionalSize);

            // Use the right most slot for our requirement, then cut up the rest of the space
            auto& rightmostFreeSlot = _slots[rightmostFreeSlotIndex];

            assert(rightmostFreeSlot.Size < requiredSize); // otherwise we've run wrong above

            auto remainingSize = rightmostFreeSlot.Size + additionalSize - requiredSize;

            rightmostFreeSlot.Occupied = true;
            rightmostFreeSlot.Size = requiredSize;

            _slots.emplace_back(SlotInfo
            {
                false,
                rightmostFreeSlot.Offset + rightmostFreeSlot.Size,
                remainingSize,
            });

            return rightmostFreeSlotIndex;
        }
    };

    Buffer<ArbitraryMeshVertex> _vertexBuffer;
    Buffer<unsigned int> _indexBuffer;

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
