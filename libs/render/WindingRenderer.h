#pragma once

#include "igl.h"
#include "irender.h"
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
    virtual void renderAllWindings(const RenderInfo& info) = 0;
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

    IGeometryStore& _geometryStore;
    Shader* _owningShader;

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
        IRenderEntity* renderEntity = nullptr;
    };

    std::vector<SlotMapping> _slots;
    static constexpr std::size_t InvalidSlotMapping = std::numeric_limits<std::size_t>::max();
    std::size_t _freeSlotMappingHint;

    std::size_t _windingCount;

    class WindingGroup :
        public IRenderableObject
    {
    private:
        WindingRenderer& _owner;

        std::set<std::size_t> _slotIndices;
        bool _surfaceNeedsRebuild;
        AABB _bounds;
        bool _boundsNeedUpdate;

        IGeometryStore::Slot _geometrySlot;
        std::size_t _pushedVertexCount;
        std::size_t _pushedIndexCount;

        static constexpr auto InvalidGeometrySlot = std::numeric_limits<IGeometryStore::Slot>::max();

        sigc::signal<void> _sigBoundsChanged;
    public:
        WindingGroup(WindingRenderer& owner) :
            _owner(owner),
            _surfaceNeedsRebuild(true),
            _boundsNeedUpdate(true),
            _geometrySlot(InvalidGeometrySlot),
            _pushedVertexCount(0),
            _pushedIndexCount(0)
        {}

        void addWinding(std::size_t slotMappingIndex)
        {
            _slotIndices.insert(slotMappingIndex);
            _surfaceNeedsRebuild = true;
            _sigBoundsChanged.emit();
        }

        void updateWinding(std::size_t slotMappingIndex)
        {
            _surfaceNeedsRebuild = true;
            _sigBoundsChanged.emit();
        }

        void removeWinding(std::size_t slotMappingIndex)
        {
            _slotIndices.erase(slotMappingIndex);
            _surfaceNeedsRebuild = true;
            _sigBoundsChanged.emit();
        }

        bool empty() const
        {
            return _slotIndices.empty();
        }

        bool isVisible() override
        {
            return !empty();
        }

        const Matrix4& getObjectTransform() override
        {
            static Matrix4 _identity = Matrix4::getIdentity();
            return _identity;
        }

        const AABB& getObjectBounds() override
        {
            if (_surfaceNeedsRebuild)
            {
                _boundsNeedUpdate = true;
                _surfaceNeedsRebuild = false;

                std::vector<ArbitraryMeshVertex> vertices;
                vertices.reserve(_slotIndices.size() * 6); // reserve 6 vertices per winding

                std::vector<unsigned int> indices;
                indices.reserve(_slotIndices.size() * 6); // reserve 6 indices per winding

                auto indexInserter = std::back_inserter(indices);
                auto vertexInserter = std::back_inserter(vertices);

                // Rebuild the whole surface
                for (auto slotIndex : _slotIndices)
                {
                    const auto& slot = _owner._slots[slotIndex];
                    auto& buffer = _owner._buckets[slot.bucketIndex];
                    
                    auto firstVertex = static_cast<unsigned int>(vertices.size());
                    auto windingSize = buffer.buffer.getWindingSize();

                    WindingIndexer_Triangles::GenerateAndAssignIndices(
                        indexInserter, windingSize, firstVertex);

                    auto sourceVertices = buffer.buffer.getVertices().begin() + (slot.slotNumber * windingSize);
                    std::copy(sourceVertices, sourceVertices + windingSize, vertexInserter);
                }

                auto sizeDiffers = _pushedVertexCount != vertices.size() || _pushedIndexCount != indices.size();

                if (_geometrySlot != InvalidGeometrySlot && sizeDiffers)
                {
                    _owner._geometryStore.deallocateSlot(_geometrySlot);
                    _geometrySlot = InvalidGeometrySlot;
                    _pushedVertexCount = 0;
                    _pushedIndexCount = 0;
                }

                if (!indices.empty())
                {
                    if (!sizeDiffers && _geometrySlot != InvalidGeometrySlot)
                    {
                        _owner._geometryStore.updateData(_geometrySlot, vertices, indices);
                    }
                    else
                    {
                        _geometrySlot = _owner._geometryStore.allocateSlot(vertices, indices);
                        _pushedVertexCount = vertices.size();
                        _pushedIndexCount = indices.size();
                    }
                }
            }

            if (_boundsNeedUpdate)
            {
                _boundsNeedUpdate = false;
                _bounds = _owner._geometryStore.getBounds(_geometrySlot);
            }

            return _bounds;
        }

        sigc::signal<void>& signal_boundsChanged() override 
        {
            return _sigBoundsChanged;
        }

        IGeometryStore::Slot getStorageLocation() override
        {
            return _geometrySlot;
        }
    };

    class EntityWindings
    {
    private:
        WindingRenderer& _owner;

        std::map<IRenderEntity*, std::shared_ptr<WindingGroup>> _windingsByEntity;

    public:
        EntityWindings(WindingRenderer& owner) :
            _owner(owner)
        {}

        void addWinding(std::size_t slotMappingIndex)
        {
            const auto& slot = _owner._slots[slotMappingIndex];

            // Find or create a surface for the entity
            auto existing = _windingsByEntity.find(slot.renderEntity);
            
            if (existing == _windingsByEntity.end())
            {
                existing = _windingsByEntity.emplace(slot.renderEntity, 
                    std::make_shared<WindingGroup>(_owner)).first;

                // New surface, register this with the entity
                slot.renderEntity->addRenderable(existing->second, _owner._owningShader);
            }

            existing->second->addWinding(slotMappingIndex);
        }

        void updateWinding(std::size_t slotMappingIndex)
        {
            const auto& slot = _owner._slots[slotMappingIndex];
            _windingsByEntity[slot.renderEntity]->updateWinding(slotMappingIndex);
        }

        void removeWinding(std::size_t slotMappingIndex)
        {
            const auto& slot = _owner._slots[slotMappingIndex];

            auto& group = _windingsByEntity[slot.renderEntity];
            group->removeWinding(slotMappingIndex);
            
            if (group->empty())
            {
                slot.renderEntity->removeRenderable(group);
                _windingsByEntity.erase(slot.renderEntity);
            }
        }
    };

    std::unique_ptr<EntityWindings> _entitySurfaces;

public:
    WindingRenderer(IGeometryStore& geometryStore, Shader* owningShader) :
        _geometryStore(geometryStore),
        _owningShader(owningShader),
        _windingCount(0),
        _freeSlotMappingHint(InvalidSlotMapping)
    {
        if (RenderingTraits<WindingIndexerT>::SupportsEntitySurfaces())
        {
            _entitySurfaces.reset(new EntityWindings(*this));
        }
    }

    bool empty() const
    {
        return _windingCount == 0;
    }

    Slot addWinding(const std::vector<ArbitraryMeshVertex>& vertices, IRenderEntity* entity) override
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
        _buckets[bucketIndex].pendingDeletions.push_back(slotMapping.slotNumber);

        // Invalidate the slot mapping
        slotMapping.bucketIndex = InvalidBucketIndex;
        slotMapping.slotNumber = InvalidVertexBufferSlot;
        slotMapping.renderEntity = nullptr;

        // Update the free slot hint, for the next round we allocate one
        if (slot < _freeSlotMappingHint)
        {
            _freeSlotMappingHint = slot;
        }

        --_windingCount;
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

        if (RenderingTraits<WindingIndexerT>::SupportsEntitySurfaces())
        {
            _entitySurfaces->updateWinding(slot);
        }
    }

    void renderAllWindings(const RenderInfo& info) override
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

        for (auto bucketIndex = 0; bucketIndex < _buckets.size(); ++bucketIndex)
        {
            auto& bucket = _buckets[bucketIndex];
            commitDeletions(bucketIndex);

            if (bucket.buffer.getVertices().empty()) continue;

            const auto& vertices = bucket.buffer.getVertices();
            const auto& indices = bucket.buffer.getIndices();

            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().vertex);

            if (info.checkFlag(RENDER_BUMP))
            {
                glVertexAttribPointer(GLProgramAttribute::Normal, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().normal);
                glVertexAttribPointer(GLProgramAttribute::TexCoord, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().texcoord);
                glVertexAttribPointer(GLProgramAttribute::Tangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().tangent);
                glVertexAttribPointer(GLProgramAttribute::Bitangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().bitangent);
            }
            else
            {
                glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().texcoord);
                glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().normal);
            }
            
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
