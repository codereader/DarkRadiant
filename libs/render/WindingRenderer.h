#pragma once

#include "igl.h"
#include <limits>
#include "iwindingrenderer.h"
#include "CompactWindingVertexBuffer.h"
#include "debugging/gl.h"

namespace render
{

class WindingRenderer :
    public IWindingRenderer
{
private:
    using VertexBuffer = CompactWindingVertexBuffer<ArbitraryMeshVertex>;

    // Maintain one bucket per winding size, allocated on demand
    std::vector<VertexBuffer> _buckets;

    using BucketIndex = std::uint16_t;
    static constexpr BucketIndex InvalidBucketIndex = std::numeric_limits<BucketIndex>::max();
    static constexpr VertexBuffer::Slot InvalidVertexBufferSlot = std::numeric_limits<VertexBuffer::Slot>::max();

    // Stores the indices to a winding slot into a bucket, client code receives an index to a SlotMapping
    struct SlotMapping
    {
        BucketIndex bucketIndex = InvalidBucketIndex;
        VertexBuffer::Slot slotNumber = InvalidVertexBufferSlot;
    };

    std::vector<SlotMapping> _slots;
    static constexpr std::uint32_t InvalidSlotMapping = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t _freeSlotMappingHint;

    std::size_t _windings;

public:
    WindingRenderer() :
        _windings(0),
        _freeSlotMappingHint(InvalidSlotMapping)
    {}

    bool empty() const
    {
        return _windings == 0;
    }

    Slot addWinding(const std::vector<ArbitraryMeshVertex>& vertices) override
    {
        auto windingSize = vertices.size();

        if (windingSize >= std::numeric_limits<BucketIndex>::max()) throw std::logic_error("Winding too large");

        // Get the Bucket this Slot is referring to
        auto bucketIndex = getBucketIndexForWindingSize(windingSize);
        auto& bucket = ensureBucketForWindingSize(windingSize);

        auto bufferSlot = _buckets[bucketIndex].pushWinding(vertices);

        // Allocate a new slot descriptor, we can't hand out absolute indices to clients
        auto slotMappingIndex = getNextFreeSlotMapping();

        auto& slotMapping = _slots[slotMappingIndex];
        slotMapping.occupied = true;
        slotMapping.slotNumber = bufferSlot;

        ++_windings;

        return getSlot(bucketIndex, slotMappingIndex);
    }

    void removeWinding(Slot slot) override
    {
        // Get the bucket and mapping indices from the encoded Slot
        auto bucket = getBucketIndex(slot);
        auto slotMappingIndex = getSlotMappingIndex(slot);

        auto& slotMapping = _slots[slotMappingIndex];

        assert(slotMapping.occupied);

        // Remove the winding from the bucket
        _buckets[bucket].removeWinding(slotMapping.slotNumber);

        // Invalidate the slot mapping
        slotMapping.occupied = false;
        slotMapping.slotNumber = InvalidSlotMapping;

        // Update the value in other slot mappings, now that the bucket shrunk


        // Update the free slot hint
        if (slotMappingIndex < _freeSlotMappingHint)
        {
            _freeSlotMappingHint = slotMappingIndex;
        }

        --_windings;
    }

    void updateWinding(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices) override
    {
        auto bucketIndex = getBucketIndex(slot);
        auto slotMappingIndex = getSlotMappingIndex(slot);

        auto& bucket = _buckets[bucketIndex];

        if (bucket.getWindingSize() != vertices.size())
        {
            throw std::logic_error("Winding size changes are not supported through updateWinding.");
        }

        assert(_slots[slotMappingIndex].occupied);
        auto bufferSlot = _slots[slotMappingIndex].slotNumber;
        bucket.replaceWinding(bufferSlot, vertices);
    }

    void render()
    {
        for (const auto& bucket : _buckets)
        {
            if (bucket.getVertices().empty()) continue;

            const auto& vertices = bucket.getVertices();
            const auto& indices = bucket.getIndices();

            glDisableClientState(GL_COLOR_ARRAY);
            glColor3f(1, 1, 1);

            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().vertex);
            glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().texcoord);
            glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().normal);
            
            glDrawElements(GL_TRIANGLES, GLint(indices.size()), GL_UNSIGNED_INT, &indices.front());

            debug::checkGLErrors();
        }
    }

private:
    std::uint32_t getNextFreeSlotMapping()
    {
        auto numSlots = static_cast<std::uint32_t>(_slots.size());

        for (auto i = _freeSlotMappingHint; i < numSlots; ++i)
        {
            if (!_slots[i].occupied)
            {
                _freeSlotMappingHint = i + 1; // start searching here next time
                return i;
            }
        }

        _slots.emplace_back();
        return numSlots; // == the size before we emplaced the new slot
    }

    inline static BucketIndex getBucketIndex(IWindingRenderer::Slot slot)
    {
        // Highest part of the slot stores the bucket index
        // The lower bytes are the index within the bucket
        constexpr auto BitsToShiftRight = (sizeof(IWindingRenderer::Slot) - sizeof(BucketIndex)) << 3;

        return slot >> BitsToShiftRight;
    }

    inline static std::uint32_t getSlotMappingIndex(IWindingRenderer::Slot slot)
    {
        constexpr auto BitsToShiftLeft = (sizeof(IWindingRenderer::Slot) - sizeof(BucketIndex)) << 3;
        constexpr auto SlotMappingMask = ~(static_cast<IWindingRenderer::Slot>(std::numeric_limits<BucketIndex>::max()) << BitsToShiftLeft);
        return static_cast<std::uint32_t>(slot & SlotMappingMask);
    }

    inline static IWindingRenderer::Slot getSlot(BucketIndex bucketIndex, std::uint32_t slotMappingIndex)
    {
        constexpr auto BitsToShiftLeft = (sizeof(IWindingRenderer::Slot) - sizeof(BucketIndex)) << 3;
        return (static_cast<IWindingRenderer::Slot>(bucketIndex) << BitsToShiftLeft) + slotMappingIndex;
    }

    inline static BucketIndex getBucketIndexForWindingSize(std::size_t windingSize)
    {
        // Since there are no windings with sizes 0, 1, 2, we can deduct 3 to get the bucket index
        if (windingSize < 3) throw std::logic_error("No winding sizes < 3 are supported");

        return static_cast<BucketIndex>(windingSize - 3);
    }

    VertexBuffer& ensureBucketForWindingSize(std::size_t windingSize)
    {
        auto bucketIndex = getBucketIndexForWindingSize(windingSize);

        // Keep adding buckets until we have the matching one
        while (bucketIndex >= _buckets.size())
        {
            auto nextWindingSize = _buckets.size() + 3;
            _buckets.emplace_back(nextWindingSize);
        }

        return _buckets.at(bucketIndex);
    }
};

}
