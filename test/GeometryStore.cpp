#include "gtest/gtest.h"

#include <limits>
#include "render/GeometryStore.h"

namespace test
{

class NullSyncObjectProvider final :
    public render::ISyncObjectProvider
{
public:
    std::size_t invocationCount = 0;

    render::ISyncObject::Ptr createSyncObject() override
    {
        ++invocationCount;
        return {};
    }

    static NullSyncObjectProvider& Instance()
    {
        static NullSyncObjectProvider _instance;
        return _instance;
    }
};

namespace
{

inline MeshVertex createNthVertex(int n, int id, std::size_t size)
{
    auto offset = static_cast<double>(n + size * id);

    return MeshVertex(
        { offset + 0.0, offset + 0.5, offset + 0.3 },
        { 0, 0, offset + 0.0 },
        { offset + 0.0, -offset + 0.0 }
    );
}

inline std::vector<MeshVertex> generateVertices(int id, std::size_t size)
{
    std::vector<MeshVertex> vertices;

    for (int i = 0; i < size; ++i)
    {
        vertices.emplace_back(createNthVertex(i, id, size));
    }

    return vertices;
}

// Generates 3 indices per vertex, without any special meaning
inline std::vector<unsigned int> generateIndices(const std::vector<MeshVertex>& vertices)
{
    std::vector<unsigned int> indices;

    for (int i = 0; i < vertices.size(); ++i)
    {
        indices.emplace_back(i);
        indices.emplace_back(static_cast<unsigned int>((i + 1) % vertices.size()));
        indices.emplace_back(static_cast<unsigned int>((i + 2) % vertices.size()));
    }

    return indices;
}

inline void verifyAllocation(render::IGeometryStore& store, render::IGeometryStore::Slot slot,
    const std::vector<MeshVertex>& vertices, const std::vector<unsigned int>& indices)
{
    auto renderParms = store.getRenderParameters(slot);

    auto expectedIndex = indices.begin();
    auto firstVertex = renderParms.bufferStart + renderParms.firstVertex;

    EXPECT_EQ(renderParms.indexCount, indices.size()) << "Index count mismatch";

    for (auto idxPtr = renderParms.firstIndex; idxPtr < renderParms.firstIndex + renderParms.indexCount; ++idxPtr)
    {
        auto index = *idxPtr;
        EXPECT_EQ(index, *expectedIndex) << "Index disorder";

        // Pick the vertex from our local expectation
        const auto& expectedVertex = vertices.at(index);

        // Pick the vertex from the stored set
        const auto& vertex = *(firstVertex + index);

        EXPECT_TRUE(math::isNear(vertex.vertex, expectedVertex.vertex, 0.01)) << "Vertex data mismatch";
        EXPECT_TRUE(math::isNear(vertex.texcoord, expectedVertex.texcoord, 0.01)) << "Texcoord data mismatch";
        EXPECT_TRUE(math::isNear(vertex.normal, expectedVertex.normal, 0.01)) << "Texcoord data mismatch";

        ++expectedIndex;
    }
}

struct Allocation
{
    render::IGeometryStore::Slot slot;
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    bool operator<(const Allocation& other) const
    {
        return slot < other.slot;
    }
};

}

TEST(GeometryStore, AllocateAndDeallocate)
{
    render::GeometryStore store(NullSyncObjectProvider::Instance());

    std::vector<render::IGeometryStore::Slot> allocatedSlots;

    // Allocate 10 slots of various sizes
    for (auto i = 0; i < 10; ++i)
    {
        auto slot = store.allocateSlot((i + 5) * 20, (i + 5) * 23);
        EXPECT_NE(slot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

        allocatedSlots.push_back(slot);
    }

    for (auto slot : allocatedSlots)
    {
        EXPECT_NO_THROW(store.deallocateSlot(slot));
    }
}

TEST(GeometryStore, UpdateData)
{
    render::GeometryStore store(NullSyncObjectProvider::Instance());

    std::set<Allocation> allocations;

    // Allocate 10 slots of various sizes, store some data in there
    for (auto i = 0; i < 10; ++i)
    {
        auto vertices = generateVertices(i, (i + 5) * 20);
        auto indices = generateIndices(vertices);

        auto slot = store.allocateSlot(vertices.size(), indices.size());
        EXPECT_NE(slot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

        // Uploading the data should succeed
        EXPECT_NO_THROW(store.updateData(slot, vertices, indices));

        allocations.emplace(Allocation{ slot, vertices, indices });

        // Verify the data after each allocation, it should not affect the others
        for (auto allocation : allocations)
        {
            verifyAllocation(store, allocation.slot, allocation.vertices, allocation.indices);
        }
    }

    // Verify the data
    for (auto allocation : allocations)
    {
        verifyAllocation(store, allocation.slot, allocation.vertices, allocation.indices);
    }

    // Now de-allocate one slot after the other and verify the remaining ones
    while (!allocations.empty())
    {
        auto slot = allocations.begin()->slot;
        allocations.erase(allocations.begin());

        EXPECT_NO_THROW(store.deallocateSlot(slot));

        // Verify the remaining slots, they should still be intact
        for (auto allocation : allocations)
        {
            verifyAllocation(store, allocation.slot, allocation.vertices, allocation.indices);
        }
    }
}

TEST(GeometryStore, UpdateSubData)
{
    render::GeometryStore store(NullSyncObjectProvider::Instance());

    std::set<Allocation> allocations;

    // Allocate 10 slots of various sizes, store some data in there
    auto margin = 13;

    for (auto i = 0; i < 10; ++i)
    {
        auto vertices = generateVertices(13, 17 * 20);
        auto indices = generateIndices(vertices);

        auto slot = store.allocateSlot(vertices.size() + margin, indices.size() + margin);
        EXPECT_NE(slot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

        // We locally keep track of what the data should look like in the store
        std::vector<MeshVertex> localVertexCopy(vertices.size());
        std::vector<unsigned int> localIndexCopy(indices.size());

        // Upload part of the data (with some increasing offset)
        for (auto offset = 0; offset < margin; ++offset)
        {
            EXPECT_NO_THROW(store.updateSubData(slot, offset, vertices, offset, indices));

            // Update our local copy accordingly
            localVertexCopy.resize(vertices.size() + offset);
            localIndexCopy.resize(indices.size() + offset);

            std::copy(vertices.begin(), vertices.end(), localVertexCopy.begin() + offset);
            std::copy(indices.begin(), indices.end(), localIndexCopy.begin() + offset);

            verifyAllocation(store, slot, localVertexCopy, localIndexCopy);
        }

        // Finally, upload the whole data
        store.updateData(slot, vertices, indices);

        allocations.emplace(Allocation{ slot, vertices, indices });

        // Verify the data after each round, it should not affect the other data
        for (auto allocation : allocations)
        {
            verifyAllocation(store, allocation.slot, allocation.vertices, allocation.indices);
        }
    }

    // Verify the data
    for (auto allocation : allocations)
    {
        verifyAllocation(store, allocation.slot, allocation.vertices, allocation.indices);
    }

    // Now de-allocate one slot after the other and verify the remaining ones
    while (!allocations.empty())
    {
        auto slot = allocations.begin()->slot;
        allocations.erase(allocations.begin());

        EXPECT_NO_THROW(store.deallocateSlot(slot));

        // Verify the remaining slots, they should still be intact
        for (auto allocation : allocations)
        {
            verifyAllocation(store, allocation.slot, allocation.vertices, allocation.indices);
        }
    }
}

TEST(GeometryStore, ResizeData)
{
    render::GeometryStore store(NullSyncObjectProvider::Instance());

    // Allocate a few dummy slots
    store.allocateSlot(17, 27);
    store.allocateSlot(31, 67);
    store.allocateSlot(5, 37);

    // Generate an indexed vertex set
    auto vertices = generateVertices(13, 17 * 20);
    auto indices = generateIndices(vertices);

    auto slot = store.allocateSlot(vertices.size(), indices.size());
    EXPECT_NE(slot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";
    
    // Store everything into the buffer
    store.updateData(slot, vertices, indices);

    // We locally keep track of what the data should look like in the store
    std::vector<MeshVertex> localVertexCopy = vertices;
    std::vector<unsigned int> localIndexCopy = indices;

    // Reduce the data in the allocation, step by step
    auto newVertexSize = localVertexCopy.size();
    auto newIndexSize = localIndexCopy.size();

    auto steps = std::min(newIndexSize, newVertexSize);
    EXPECT_GT(steps, 4) << "Too few data elements";
    steps -= 4;

    for (auto i = 0; i < steps; ++i)
    {
        // Cut off one index at the end
        // Keep the vertex buffer intact, we don't want out-of-bounds errors
        localIndexCopy.resize(localIndexCopy.size() - 1);
        --newVertexSize;

        EXPECT_NO_THROW(store.resizeData(slot, newVertexSize, localIndexCopy.size()));

        verifyAllocation(store, slot, localVertexCopy, localIndexCopy);
    }
}

TEST(GeometryStore, SyncObjectAcquisition)
{
    render::GeometryStore store(NullSyncObjectProvider::Instance());

    store.onFrameStart();
    store.onFrameFinished();

    EXPECT_EQ(NullSyncObjectProvider::Instance().invocationCount, 1) <<
        "GeometryStore should have acquired one sync object";
}

}
