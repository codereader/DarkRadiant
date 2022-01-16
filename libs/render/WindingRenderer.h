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

// Traits class to retrieve the GLenum render mode based on the indexer type
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

template<>
struct RenderingTraits<WindingIndexer_Polygon>
{
    constexpr static GLenum Mode() { return GL_POLYGON; }
};

template<class WindingIndexerT>
class WindingRenderer :
    public IBackendWindingRenderer
{
private:
    using VertexBuffer = CompactWindingVertexBuffer<ArbitraryMeshVertex, WindingIndexerT>;

    struct Bucket
    {
        Bucket(std::size_t size) :
            buffer(size)
        {}

        VertexBuffer buffer;
        std::vector<typename VertexBuffer::Slot> pendingDeletions;
    };

    // Maintain one bucket per winding size, allocated on demand
    std::vector<Bucket> _buckets;

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

        // Allocate a new slot descriptor, we can't hand out absolute indices to clients
        auto slotMappingIndex = allocateSlotMapping();

        auto& slotMapping = _slots[slotMappingIndex];
        slotMapping.bucketIndex = bucketIndex;

        // Check if we have a free slot in this buffer (marked for deletion)
        if (!bucket.pendingDeletions.empty())
        {
            slotMapping.slotNumber = bucket.pendingDeletions.back();
            bucket.pendingDeletions.pop_back();

            // Use the replace method to load the data
            bucket.buffer.replaceWinding(slotMapping.slotNumber, vertices);
        }
        else
        {
            // No deleted slot available, allocate a new one
            slotMapping.slotNumber = bucket.buffer.pushWinding(vertices);
        }

        ++_windings;

        return slotMappingIndex;
    }

    void removeWinding(Slot slot) override
    {
        assert(slot < _slots.size());
        auto& slotMapping = _slots[slot];

        auto bucketIndex = slotMapping.bucketIndex;
        assert(bucketIndex != InvalidBucketIndex);

#if 1
        // Mark this winding slot as pending for deletion
        _buckets[bucketIndex].pendingDeletions.push_back(slotMapping.slotNumber);
#else
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
#endif

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

        if (bucket.buffer.getWindingSize() != vertices.size())
        {
            throw std::logic_error("Winding size changes are not supported through updateWinding.");
        }

        bucket.buffer.replaceWinding(slotMapping.slotNumber, vertices);
    }

    void renderAllWindings() override
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

        glFrontFace(GL_CCW);

        for (auto bucketIndex = 0; bucketIndex < _buckets.size(); ++bucketIndex)
        {
            auto& bucket = _buckets[bucketIndex];
            commitDeletions(bucketIndex);

            if (bucket.buffer.getVertices().empty()) continue;

            const auto& vertices = bucket.buffer.getVertices();
            const auto& indices = bucket.buffer.getIndices();

            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().vertex);
            glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().texcoord);
            glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().normal);
            
            auto primitiveMode = RenderingTraits<WindingIndexerT>::Mode();
            glDrawElements(primitiveMode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, &indices.front());

            debug::checkGLErrors();
        }

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    void renderWinding(IWindingRenderer::RenderMode mode, IWindingRenderer::Slot slot) override
    {
        assert(slot < _slots.size());
        auto& slotMapping = _slots[slot];

        assert(slotMapping.bucketIndex != InvalidBucketIndex);
        auto& bucket = _buckets[slotMapping.bucketIndex];

        commitDeletions(slotMapping.bucketIndex);

        const auto& vertices = bucket.buffer.getVertices();
        const auto& indices = bucket.buffer.getIndices();

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

        glFrontFace(GL_CCW);

        glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().vertex);
        glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().texcoord);
        glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().normal);

        if (mode == IWindingRenderer::RenderMode::Triangles)
        {
            renderElements<WindingIndexer_Triangles>(bucket.buffer, slotMapping.slotNumber);
        }
        else if (mode == IWindingRenderer::RenderMode::Polygon)
        {
            renderElements<WindingIndexer_Polygon>(bucket.buffer, slotMapping.slotNumber);
        }

        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

private:
    void commitDeletions(std::size_t bucketIndex)
    {
        auto& bucket = _buckets[bucketIndex];

        if (bucket.pendingDeletions.empty()) return;

        std::sort(bucket.pendingDeletions.begin(), bucket.pendingDeletions.end());

#if 1
        // Remove the winding from the bucket
        bucket.buffer.removeWindings(bucket.pendingDeletions);

        // A mapping to quickly know which mapping has been shifted by how many positions
        std::map<typename VertexBuffer::Slot, typename VertexBuffer::Slot> offsets;
        auto offsetToApply = 0;

        for (auto removed : bucket.pendingDeletions)
        {
            offsets[removed] = offsetToApply++;
        }

        auto maxOffsetToApply = offsetToApply;

        for (auto& mapping : _slots)
        {
            // Every index in the same bucket beyond the first removed winding needs to be shifted
            if (mapping.bucketIndex != bucketIndex || mapping.slotNumber == InvalidVertexBufferSlot)
            {
                continue;
            }

            // lower_bound yields the item that is equal to or larger
            auto offset = offsets.lower_bound(mapping.slotNumber);

            if (offset != offsets.end())
            {
                mapping.slotNumber -= offset->second;
            }
            else
            {
                mapping.slotNumber -= maxOffsetToApply;
            }
        }
#else
        for (auto s = bucket.pendingDeletions.rbegin(); s != bucket.pendingDeletions.rend(); ++s)
        {
            auto slotNumber = *s;

            // Remove the winding from the bucket
            bucket.buffer.removeWinding(slotNumber);

            // Update the value in other slot mappings, now that the bucket shrank
            for (auto& mapping : _slots)
            {
                // Every index in the same bucket beyond the removed winding needs to be shifted to left
                if (mapping.bucketIndex == bucketIndex && mapping.slotNumber > slotNumber)
                {
                    --mapping.slotNumber;
                }
            }
        }
#endif
        bucket.pendingDeletions.clear();
    }

    template<class CustomWindingIndexerT>
    void renderElements(const VertexBuffer& buffer, typename VertexBuffer::Slot slotNumber) const
    {
        std::vector<unsigned int> indices;
        indices.reserve(CustomWindingIndexerT::GetNumberOfIndicesPerWinding(buffer.getWindingSize()));

        auto firstVertex = static_cast<unsigned int>(buffer.getWindingSize() * slotNumber);
        CustomWindingIndexerT::GenerateAndAssignIndices(std::back_inserter(indices), buffer.getWindingSize(), firstVertex);
        auto mode = RenderingTraits<CustomWindingIndexerT>::Mode();

        glDrawElements(mode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, &indices.front());
    }

    IWindingRenderer::Slot allocateSlotMapping()
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

    Bucket& ensureBucketForWindingSize(std::size_t windingSize)
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
