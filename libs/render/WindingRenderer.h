#pragma once

#include "igl.h"
#include <limits>
#include "iwindingrenderer.h"
#include "CompactWindingVertexBuffer.h"
#include "debugging/gl.h"

namespace render
{

class IBackendWindingRenderer :
    public IWindingRenderer
{
public:
    virtual ~IBackendWindingRenderer() 
    {}

    // Returns true if the vertex buffers are empty
    virtual bool empty() const = 0;

    // Issues the openGL calls to render the vertex buffers
    virtual void renderAllWindings() = 0;
};

template<class WindingIndexerT>
class WindingRenderer :
    public IBackendWindingRenderer
{
private:
    template<typename IndexerT> struct RenderingTraits
    {};

    template<>
    struct RenderingTraits<WindingIndexer_Lines>
    {
        constexpr static GLenum Mode() { return GL_LINES; }
    };

    template<>
    struct RenderingTraits<WindingIndexer_Triangles>
    {
        constexpr static GLenum Mode() { return GL_TRIANGLES; }
    };

private:
    using VertexBuffer = CompactWindingVertexBuffer<ArbitraryMeshVertex, WindingIndexerT>;

    // Maintain one bucket per winding size, allocated on demand
    std::vector<VertexBuffer> _buckets;

    using BucketIndex = std::uint16_t;
    static constexpr BucketIndex InvalidBucketIndex = std::numeric_limits<BucketIndex>::max();
    static constexpr typename VertexBuffer::Slot InvalidVertexBufferSlot = std::numeric_limits<typename VertexBuffer::Slot>::max();

    // Stores the indices to a winding slot into a bucket, client code receives an index to a SlotMapping
    struct SlotMapping
    {
        BucketIndex bucketIndex = InvalidBucketIndex;
        typename VertexBuffer::Slot slotNumber = InvalidVertexBufferSlot;
    };

    std::vector<SlotMapping> _slots;
    static constexpr std::size_t InvalidSlotMapping = std::numeric_limits<std::size_t>::max();
    std::size_t _freeSlotMappingHint;

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
        slotMapping.bucketIndex = bucketIndex;
        slotMapping.slotNumber = bufferSlot;

        ++_windings;

        return slotMappingIndex;
    }

    void removeWinding(Slot slot) override
    {
        assert(slot < _slots.size());
        auto& slotMapping = _slots[slot];

        auto bucketIndex = slotMapping.bucketIndex;
        assert(bucketIndex != InvalidBucketIndex);

        // Remove the winding from the bucket
        _buckets[bucketIndex].removeWinding(slotMapping.slotNumber);

        // Update the value in other slot mappings, now that the bucket shrunk
        for (auto& mapping : _slots)
        {
            // Every index in the same bucket beyond the removed winding needs to be shifted to left
            if (mapping.bucketIndex == bucketIndex && mapping.slotNumber > slotMapping.slotNumber)
            {
                --mapping.slotNumber;
            }
        }

        // Invalidate the slot mapping
        slotMapping.bucketIndex = InvalidBucketIndex;
        slotMapping.slotNumber = InvalidVertexBufferSlot;

        // Update the free slot hint, for the next round we allocate one
        if (slot < _freeSlotMappingHint)
        {
            _freeSlotMappingHint = slot;
        }

        --_windings;
    }

    void updateWinding(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices) override
    {
        assert(slot < _slots.size());
        auto& slotMapping = _slots[slot];

        assert(slotMapping.bucketIndex != InvalidBucketIndex);

        auto& bucket = _buckets[slotMapping.bucketIndex];

        if (bucket.getWindingSize() != vertices.size())
        {
            throw std::logic_error("Winding size changes are not supported through updateWinding.");
        }

        bucket.replaceWinding(slotMapping.slotNumber, vertices);
    }

    void renderAllWindings() override
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
            
            auto primitiveMode = RenderingTraits<WindingIndexerT>::Mode();
            glDrawElements(primitiveMode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, &indices.front());

            debug::checkGLErrors();
        }
    }

private:
    IWindingRenderer::Slot getNextFreeSlotMapping()
    {
        auto numSlots = static_cast<IWindingRenderer::Slot>(_slots.size());

        for (auto i = _freeSlotMappingHint; i < numSlots; ++i)
        {
            if (_slots[i].bucketIndex == InvalidBucketIndex)
            {
                _freeSlotMappingHint = i + 1; // start searching here next time
                return i;
            }
        }

        _slots.emplace_back();
        return numSlots; // == the size before we emplaced the new slot
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
