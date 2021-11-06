#pragma once

#include <limits>
#include "iwindingrenderer.h"
#include "CompactWindingVertexBuffer.h"

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

public:
    Slot addWinding(const std::vector<ArbitraryMeshVertex>& vertices) override
    {
        auto windingSize = vertices.size();

        if (windingSize >= std::numeric_limits<BucketIndex>::max()) throw std::logic_error("Winding too large");

        // Get the Bucket this Slot is referring to
        auto bucketIndex = getBucketIndexForWindingSize(windingSize);
        auto& bucket = ensureBucketForWindingSize(windingSize);
        auto bufferSlot = _buckets[bucketIndex].pushWinding(vertices);

        return getSlot(bucketIndex, bufferSlot);
    }

    void removeWinding(Slot slot) override
    {
        auto bucket = getBucketIndex(slot);
        auto bufferSlot = getVertexBufferSlot(slot);

        _buckets[bucket].removeWinding(bufferSlot);
    }

    void updateWinding(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices) override
    {
        auto bucketIndex = getBucketIndex(slot);
        auto bufferSlot = getVertexBufferSlot(slot);

        auto& bucket = _buckets[bucketIndex];

        if (bucket.getWindingSize() != vertices.size())
        {
            throw std::logic_error("Winding size changes are not supported through updateWinding.");
        }

        bucket.replaceWinding(bufferSlot, vertices);
    }

private:
    inline static BucketIndex getBucketIndex(IWindingRenderer::Slot slot)
    {
        // Highest part of the slot stores the bucket index
        // The lower bytes are the index within the bucket
        constexpr auto BitsToShiftRight = (sizeof(IWindingRenderer::Slot) - sizeof(BucketIndex)) << 3;

        return slot >> BitsToShiftRight;
    }

    inline static VertexBuffer::Slot getVertexBufferSlot(IWindingRenderer::Slot slot)
    {
        constexpr auto BitsToShiftLeft = (sizeof(IWindingRenderer::Slot) - sizeof(BucketIndex)) << 3;
        constexpr auto BufferSlotMask = ~(static_cast<IWindingRenderer::Slot>(std::numeric_limits<BucketIndex>::max()) << BitsToShiftLeft);
        return static_cast<VertexBuffer::Slot>(slot & BufferSlotMask);
    }

    inline static IWindingRenderer::Slot getSlot(BucketIndex bucketIndex, VertexBuffer::Slot bufferSlot)
    {
        constexpr auto BitsToShiftLeft = (sizeof(IWindingRenderer::Slot) - sizeof(BucketIndex)) << 3;
        return (static_cast<IWindingRenderer::Slot>(bucketIndex) << BitsToShiftLeft) + bufferSlot;
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
