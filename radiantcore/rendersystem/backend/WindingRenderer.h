#pragma once

#include "igl.h"
#include "irender.h"
#include <limits>
#include "iwindingrenderer.h"
#include "ObjectRenderer.h"
#include "render/CompactWindingVertexBuffer.h"
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

    // Submit the geometry of all windings in this renderer
    virtual void renderAllWindings() = 0;

    // Ensures that everything in the IGeometryStore is up to date
    virtual void prepareForRendering() = 0;
};

// Traits class to retrieve the GLenum render mode based on the indexer type
template<typename IndexerT> struct RenderingTraits
{};

template<>
struct RenderingTraits<WindingIndexer_Lines>
{
    constexpr static GLenum Mode() { return GL_LINES; }

    constexpr static bool SupportsEntitySurfaces() { return false; }
};

template<>
struct RenderingTraits<WindingIndexer_Triangles>
{
    constexpr static GLenum Mode() { return GL_TRIANGLES; }

    constexpr static bool SupportsEntitySurfaces() { return true; }
};

template<>
struct RenderingTraits<WindingIndexer_Polygon>
{
    constexpr static GLenum Mode() { return GL_POLYGON; }

    constexpr static bool SupportsEntitySurfaces() { return false; }
};

template<class WindingIndexerT>
class WindingRenderer final :
    public IBackendWindingRenderer
{
private:
    using VertexBuffer = CompactWindingVertexBuffer<RenderVertex, WindingIndexerT>;
    static constexpr typename VertexBuffer::Slot InvalidVertexBufferSlot = std::numeric_limits<typename VertexBuffer::Slot>::max();
    static constexpr IGeometryStore::Slot InvalidStorageHandle = std::numeric_limits<IGeometryStore::Slot>::max();

    IGeometryStore& _geometryStore;
    Shader* _owningShader;

    using BucketIndex = std::uint16_t;
    static constexpr BucketIndex InvalidBucketIndex = std::numeric_limits<BucketIndex>::max();

    // A Bucket holds all windings of a certain size (3,4,5...)
    struct Bucket
    {
        Bucket(BucketIndex bucketIndex, std::size_t size) :
            index(bucketIndex),
            buffer(size),
            storageHandle(InvalidStorageHandle),
            storageCapacity(0),
            modifiedSlotRange(InvalidVertexBufferSlot, 0)
        {}

        BucketIndex index;

        VertexBuffer buffer;
        std::vector<typename VertexBuffer::Slot> pendingDeletions;

        // The memory chunk in the central storage
        IGeometryStore::Slot storageHandle;

        // The maximum number of windings we can load in the geometry store
        std::size_t storageCapacity;

        // The lowest and highest slot numbers that have been modified since the last sync
        std::pair<typename VertexBuffer::Slot, typename VertexBuffer::Slot> modifiedSlotRange;
    };

    // Maintain one bucket per winding size, allocated on demand
    std::vector<Bucket> _buckets;

    // Stores the offset of a winding slot within a bucket, client code receives an index to a SlotMapping
    struct SlotMapping
    {
        BucketIndex bucketIndex = InvalidBucketIndex;
        typename VertexBuffer::Slot slotNumber = InvalidVertexBufferSlot;
        IRenderEntity* renderEntity = nullptr;
    };

    std::vector<SlotMapping> _slots;
    static constexpr std::size_t InvalidSlotMapping = std::numeric_limits<std::size_t>::max();
    std::size_t _freeSlotMappingHint;

    std::size_t _windingCount;

    // Represents a group of windings associated to a single entity
    // A winding is identified by its slot mapping index as used by the parent WindingRenderer
    class WindingGroup :
        public IRenderableObject
    {
    private:
        WindingRenderer& _owner;
        BucketIndex _bucketIndex;

        // The winding slot mapping indices, an index into
        // the _slots vector of the parent WindingRenderer
        std::set<Slot> _slotMappingIndices;

        bool _surfaceNeedsRebuild;
        AABB _bounds;
        bool _boundsNeedUpdate;

        IGeometryStore::Slot _indexSlot;
        IGeometryStore::Slot _vertexSlot;
        std::size_t _indexCapacity;

        sigc::signal<void> _sigBoundsChanged;
    public:
        WindingGroup(WindingRenderer& owner, BucketIndex bucketIndex) :
            _owner(owner),
            _bucketIndex(bucketIndex),
            _surfaceNeedsRebuild(true),
            _boundsNeedUpdate(true),
            _indexSlot(InvalidStorageHandle),
            _vertexSlot(InvalidStorageHandle),
            _indexCapacity(0)
        {}

        ~WindingGroup()
        {
            deallocateGeometrySlot();
        }

        void addWinding(Slot slotMappingIndex)
        {
            _slotMappingIndices.insert(slotMappingIndex);
            _surfaceNeedsRebuild = true;
            boundsChanged();
        }

        void updateWinding(Slot _)
        {
            boundsChanged();
        }

        void removeWinding(Slot slotMappingIndex)
        {
            _slotMappingIndices.erase(slotMappingIndex);
            _surfaceNeedsRebuild = true;
            boundsChanged();
        }

        bool empty() const
        {
            return _slotMappingIndices.empty();
        }

        bool isVisible() override
        {
            return !empty();
        }

        bool isOriented() override
        {
            return false;
        }

        const Matrix4& getObjectTransform() override
        {
            static Matrix4 _identity = Matrix4::getIdentity();
            return _identity;
        }

        const AABB& getObjectBounds() override
        {
            ensureSurfaceIsBuilt();

            if (_boundsNeedUpdate)
            {
                _boundsNeedUpdate = false;
                _bounds = _owner._geometryStore.getBounds(_indexSlot);
            }

            return _bounds;
        }

        sigc::signal<void>& signal_boundsChanged() override 
        {
            return _sigBoundsChanged;
        }

        IGeometryStore::Slot getStorageLocation() override
        {
            ensureSurfaceIsBuilt();

            return _indexSlot;
        }

    private:
        void boundsChanged()
        {
            _boundsNeedUpdate = true;
            _sigBoundsChanged.emit();
        }

        void ensureSurfaceIsBuilt()
        {
            if (!_surfaceNeedsRebuild) return;

            _surfaceNeedsRebuild = false;

            auto& bucket = _owner._buckets[_bucketIndex];

            // Make sure the bucket is synced with the geometry store
            _owner.ensureBucketIsReady(bucket);

            auto numIndicesPerWinding = bucket.buffer.getNumIndicesPerWinding();
            auto requiredIndexSize = _slotMappingIndices.size() * numIndicesPerWinding;

            // Have we run out of windings? Then de-allocate and we're done
            if (requiredIndexSize == 0)
            {
                deallocateGeometrySlot();
                return;
            }

            std::vector<unsigned int> indices;
            indices.reserve(requiredIndexSize);

            auto indexInserter = std::back_inserter(indices);

            // Arrange all indices into a new array
            for (auto slotMappingIndex : _slotMappingIndices)
            {
                auto& slot = _owner._slots[slotMappingIndex];
                auto sourceIndices = bucket.buffer.getIndices().begin() + (slot.slotNumber * numIndicesPerWinding);
                std::copy(sourceIndices, sourceIndices + numIndicesPerWinding, indexInserter);
            }

            // Re-allocate once we exceed the available storage
            // or if the referenced slot changed in the meantime
            if (_vertexSlot != bucket.storageHandle || _indexCapacity < indices.size())
            {
                deallocateGeometrySlot();

                // Allocate a new index remapping slot
                _indexCapacity = indices.size();
                _indexSlot = _owner._geometryStore.allocateIndexSlot(bucket.storageHandle, _indexCapacity);

                // Remember the vertex storage handle, we need to be able to detect changes later
                _vertexSlot = bucket.storageHandle;
            }

            _owner._geometryStore.updateIndexData(_indexSlot, indices);
        }

        void deallocateGeometrySlot()
        {
            if (_indexSlot == InvalidStorageHandle) return;

            _owner._geometryStore.deallocateSlot(_indexSlot);
            _indexSlot = InvalidStorageHandle;
            _vertexSlot = InvalidStorageHandle;
            _indexCapacity = 0;
        }
    };

    // Internal helper to groups windings (slots) by entities
    class EntityWindings
    {
    private:
        WindingRenderer& _owner;

        using Key = std::pair<IRenderEntity*, BucketIndex>;
        std::map<Key, std::shared_ptr<WindingGroup>> _windingMap;

    public:
        EntityWindings(WindingRenderer& owner) :
            _owner(owner)
        {}

        void addWinding(Slot slotMappingIndex)
        {
            const auto& slot = _owner._slots[slotMappingIndex];

            // Find or create a surface for the entity
            auto key = std::make_pair(slot.renderEntity, slot.bucketIndex);
            auto existing = _windingMap.find(key);
            
            if (existing == _windingMap.end())
            {
                existing = _windingMap.emplace(key,
                    std::make_shared<WindingGroup>(_owner, slot.bucketIndex)).first;

                // New surface, register this with the entity
                slot.renderEntity->addRenderable(existing->second, _owner._owningShader);
            }

            existing->second->addWinding(slotMappingIndex);
        }

        void updateWinding(Slot slotMappingIndex)
        {
            const auto& slot = _owner._slots[slotMappingIndex];

            auto key = std::make_pair(slot.renderEntity, slot.bucketIndex);
            _windingMap[key]->updateWinding(slotMappingIndex);
        }

        void removeWinding(Slot slotMappingIndex)
        {
            const auto& slot = _owner._slots[slotMappingIndex];

            auto key = std::make_pair(slot.renderEntity, slot.bucketIndex);
            auto& group = _windingMap[key];
            group->removeWinding(slotMappingIndex);
            
            if (group->empty())
            {
                slot.renderEntity->removeRenderable(group);
                _windingMap.erase(key);
            }
        }
    };

    std::unique_ptr<EntityWindings> _entitySurfaces;

    bool _geometryUpdatePending;

public:
    WindingRenderer(IGeometryStore& geometryStore, Shader* owningShader) :
        _geometryStore(geometryStore),
        _owningShader(owningShader),
        _windingCount(0),
        _freeSlotMappingHint(InvalidSlotMapping),
        _geometryUpdatePending(false)
    {
        if (RenderingTraits<WindingIndexerT>::SupportsEntitySurfaces())
        {
            _entitySurfaces = std::make_unique<EntityWindings>(*this);
        }
    }

    ~WindingRenderer()
    {
        _entitySurfaces.reset();

        // Release all storage allocations
        for (auto& bucket : _buckets)
        {
            deallocateStorage(bucket);
        }
    }

    bool empty() const override
    {
        return _windingCount == 0;
    }

    Slot addWinding(const std::vector<RenderVertex>& vertices, IRenderEntity* entity) override
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

        updateModifiedRange(bucket, slotMapping.slotNumber);

        ++_windingCount;

        if (RenderingTraits<WindingIndexerT>::SupportsEntitySurfaces())
        {
            slotMapping.renderEntity = entity;

            // Add this winding to the surface associated to the render entity
            _entitySurfaces->addWinding(slotMappingIndex);
        }

        return slotMappingIndex;
    }

    void removeWinding(Slot slot) override
    {
        assert(slot < _slots.size());
        auto& slotMapping = _slots[slot];

        auto bucketIndex = slotMapping.bucketIndex;
        assert(bucketIndex != InvalidBucketIndex);

        if (RenderingTraits<WindingIndexerT>::SupportsEntitySurfaces())
        {
            _entitySurfaces->removeWinding(slot);
        }

        // Mark this winding slot as pending for deletion
        auto& bucket = _buckets.at(bucketIndex);
        bucket.pendingDeletions.push_back(slotMapping.slotNumber);

        updateModifiedRange(bucket, slotMapping.slotNumber);

        // Invalidate the slot mapping
        slotMapping.bucketIndex = InvalidBucketIndex;
        slotMapping.slotNumber = InvalidVertexBufferSlot;
        slotMapping.renderEntity = nullptr;

        // Update the free slot hint, for the next round we allocate one
        if (slot < _freeSlotMappingHint)
        {
            _freeSlotMappingHint = slot;
        }

        if (--_windingCount == 0)
        {
            // This was the last winding in the entire renderer, run a cleanup round
            for (auto& bucket : _buckets)
            {
                ensureBucketIsReady(bucket);
            }
        }
    }

    void updateWinding(Slot slot, const std::vector<RenderVertex>& vertices) override
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

        updateModifiedRange(bucket, slotMapping.slotNumber);

        if (RenderingTraits<WindingIndexerT>::SupportsEntitySurfaces())
        {
            _entitySurfaces->updateWinding(slot);
        }
    }

    void renderAllWindings() override
    {
        assert(!_geometryUpdatePending); // prepareForRendering should have been called

        for (auto& bucket : _buckets)
        {
            if (bucket.storageHandle == InvalidStorageHandle) continue; // nothing here

            auto primitiveMode = RenderingTraits<WindingIndexerT>::Mode();
            ObjectRenderer::SubmitGeometry(bucket.storageHandle, primitiveMode, _geometryStore);
        }
    }

    void renderWinding(IWindingRenderer::RenderMode mode, IWindingRenderer::Slot slot) override
    {
        assert(!_geometryUpdatePending); // prepareForRendering should have been called

        assert(slot < _slots.size());
        auto& slotMapping = _slots[slot];

        assert(slotMapping.bucketIndex != InvalidBucketIndex);
        auto& bucket = _buckets[slotMapping.bucketIndex];

        const auto& vertices = bucket.buffer.getVertices();
        const auto& indices = bucket.buffer.getIndices();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glDisableClientState(GL_COLOR_ARRAY);

        glVertexPointer(3, GL_FLOAT, sizeof(RenderVertex), &vertices.front().vertex);
        glTexCoordPointer(2, GL_FLOAT, sizeof(RenderVertex), &vertices.front().texcoord);
        glNormalPointer(GL_FLOAT, sizeof(RenderVertex), &vertices.front().normal);

        if (mode == IWindingRenderer::RenderMode::Triangles)
        {
            renderSingleWinding<WindingIndexer_Triangles>(bucket.buffer, slotMapping.slotNumber);
        }
        else if (mode == IWindingRenderer::RenderMode::Polygon)
        {
            renderSingleWinding<WindingIndexer_Polygon>(bucket.buffer, slotMapping.slotNumber);
        }
    }

    // Ensure all data is written to the IGeometryStore
    void prepareForRendering() override
    {
        if (!_geometryUpdatePending) return;

        _geometryUpdatePending = false;

        for (auto& bucket : _buckets)
        {
            ensureBucketIsReady(bucket);
        }
    }

private:
    void ensureBucketIsReady(BucketIndex bucketIndex)
    {
        ensureBucketIsReady(_buckets[bucketIndex]);
    }

    void ensureBucketIsReady(Bucket& bucket)
    {
        commitDeletions(bucket);
        syncWithGeometryStore(bucket);
    }

    void updateModifiedRange(Bucket& bucket, typename VertexBuffer::Slot modifiedSlot)
    {
        // Update the modified range
        bucket.modifiedSlotRange.first = std::min(bucket.modifiedSlotRange.first, modifiedSlot);
        bucket.modifiedSlotRange.second = std::max(bucket.modifiedSlotRange.second, modifiedSlot);
        _geometryUpdatePending = true;
    }
    
    // Commit all local buffer changes to the geometry store
    void syncWithGeometryStore(Bucket& bucket)
    {
        if (bucket.modifiedSlotRange.first == InvalidVertexBufferSlot)
        {
            return; // no changes
        }

        auto numberOfStoredWindings = static_cast<typename VertexBuffer::Slot>(bucket.buffer.getNumberOfStoredWindings());

        if (numberOfStoredWindings == 0)
        {
            // Empty, deallocate the storage
            deallocateStorage(bucket);

            bucket.modifiedSlotRange.first = InvalidVertexBufferSlot;
            bucket.modifiedSlotRange.second = 0;
            return;
        }

        // Constrain modified range to actual bounds of our vertex storage
        if (bucket.modifiedSlotRange.first >= numberOfStoredWindings)
        {
            bucket.modifiedSlotRange.first = numberOfStoredWindings - 1;
        }

        if (bucket.modifiedSlotRange.second >= numberOfStoredWindings)
        {
            bucket.modifiedSlotRange.second = numberOfStoredWindings - 1;
        }

        const auto& vertices = bucket.buffer.getVertices();
        const auto& indices = bucket.buffer.getIndices();

        // Ensure our storage allocation is large enough
        if (bucket.storageCapacity < numberOfStoredWindings)
        {
            // (Re-)allocate a chunk that is large enough
            // Release the old one first
            deallocateStorage(bucket);
            
            bucket.storageHandle = _geometryStore.allocateSlot(vertices.size(), indices.size());
            bucket.storageCapacity = numberOfStoredWindings;
            
            _geometryStore.updateData(bucket.storageHandle, vertices, indices);
        }
        else
        {
            // Copy the modified range to the store
            // We need to set up a local copy here, this could be optimised
            // if the GeometryStore accepted iterator ranges
            std::vector<RenderVertex> vertexSubData;

            auto firstVertex = bucket.modifiedSlotRange.first * bucket.buffer.getWindingSize();
            auto highestVertex = (bucket.modifiedSlotRange.second + 1) * bucket.buffer.getWindingSize();
            vertexSubData.reserve(highestVertex - firstVertex);
            
            std::copy(vertices.begin() + firstVertex, vertices.begin() + highestVertex, std::back_inserter(vertexSubData));

            std::vector<unsigned int> indexSubData;
            auto firstIndex = bucket.modifiedSlotRange.first * bucket.buffer.getNumIndicesPerWinding();
            auto highestIndex = (bucket.modifiedSlotRange.second + 1) * bucket.buffer.getNumIndicesPerWinding();
            indexSubData.reserve(highestIndex - firstIndex);

            std::copy(indices.begin() + firstIndex, indices.begin() + highestIndex, std::back_inserter(indexSubData));

            // Upload just this subset
            _geometryStore.updateSubData(bucket.storageHandle, firstVertex, vertexSubData, firstIndex, indexSubData);

            // Shrink the storage to what we actually use
            _geometryStore.resizeData(bucket.storageHandle, vertices.size(), indices.size());
        }

        // Mark the bucket as unmodified
        bucket.modifiedSlotRange.first = InvalidVertexBufferSlot;
        bucket.modifiedSlotRange.second = 0;
    }

    void deallocateStorage(Bucket& bucket)
    {
        if (bucket.storageHandle == InvalidStorageHandle) return;

        _geometryStore.deallocateSlot(bucket.storageHandle);
        bucket.storageHandle = InvalidStorageHandle;
        bucket.storageCapacity = 0;
    }

    void commitDeletions(BucketIndex bucketIndex)
    {
        commitDeletions(_buckets[bucketIndex]);
    }

    void commitDeletions(Bucket& bucket)
    {
        if (bucket.pendingDeletions.empty()) return;

        std::sort(bucket.pendingDeletions.begin(), bucket.pendingDeletions.end());

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
            if (mapping.bucketIndex != bucket.index || mapping.slotNumber == InvalidVertexBufferSlot)
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

        bucket.pendingDeletions.clear();
    }

    template<class CustomWindingIndexerT>
    void renderSingleWinding(const VertexBuffer& buffer, typename VertexBuffer::Slot slotNumber) const
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
            _buckets.emplace_back(getBucketIndexForWindingSize(nextWindingSize), nextWindingSize);
        }

        return _buckets.at(bucketIndex);
    }
};

}
