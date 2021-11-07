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

    std::size_t _windings;

public:
    WindingRenderer() :
        _windings(0)
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
        ++_windings;

        return getSlot(bucketIndex, bufferSlot);
    }

    void removeWinding(Slot slot) override
    {
        auto bucket = getBucketIndex(slot);
        auto bufferSlot = getVertexBufferSlot(slot);
        --_windings;

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
