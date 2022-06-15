#include "gtest/gtest.h"

#include <limits>
#include <numeric>
#include <random>
#include "render/GeometryStore.h"
#include "testutil/TestBufferObjectProvider.h"
#include "testutil/TestSyncObjectProvider.h"
#include "testutil/RenderUtils.h"

namespace test
{

namespace
{

TestBufferObjectProvider _testBufferObjectProvider;

inline void verifyAllocation(render::IGeometryStore& store, render::IGeometryStore::Slot slot,
    const std::vector<render::RenderVertex>& vertices, const std::vector<unsigned int>& indices)
{
    auto renderParms = store.getRenderParameters(slot);

    auto expectedIndex = indices.begin();
    auto firstVertex = renderParms.clientBufferStart + renderParms.firstVertex;

    EXPECT_EQ(renderParms.indexCount, indices.size()) << "Index count mismatch";

    for (auto idxPtr = renderParms.clientFirstIndex; idxPtr < renderParms.clientFirstIndex + renderParms.indexCount; ++idxPtr)
    {
        auto index = *idxPtr;
        EXPECT_EQ(index, *expectedIndex) << "Index disorder";

        // Pick the vertex from our local expectation
        const auto& expectedVertex = vertices.at(index);

        // Pick the vertex from the stored set
        const auto& vertex = *(firstVertex + index);

        EXPECT_TRUE(math::isNear(vertex.vertex, expectedVertex.vertex, 0.01)) << "Vertex data mismatch";
        EXPECT_TRUE(math::isNear(vertex.texcoord, expectedVertex.texcoord, 0.01)) << "Texcoord data mismatch";
        EXPECT_TRUE(math::isNear(vertex.normal, expectedVertex.normal, 0.01)) << "Normal data mismatch";

        ++expectedIndex;
    }
}

struct Allocation
{
    render::IGeometryStore::Slot slot;
    std::vector<render::RenderVertex> vertices;
    std::vector<unsigned int> indices;

    bool operator<(const Allocation& other) const
    {
        return slot < other.slot;
    }
};

inline void verifyAllAllocations(render::IGeometryStore& store, const std::vector<Allocation>& allocations)
{
    for (auto allocation : allocations)
    {
        verifyAllocation(store, allocation.slot, allocation.vertices, allocation.indices);
    }
}

}

TEST(GeometryStore, AllocateAndDeallocate)
{
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

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
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

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
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

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
        std::vector<render::RenderVertex> localVertexCopy(vertices.size());
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
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

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
    std::vector<render::RenderVertex> localVertexCopy = vertices;
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

TEST(GeometryStore, FrameBufferSwitching)
{
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

    store.onFrameStart();

    std::vector<Allocation> allocations;

    // Allocate 10 slots of various sizes, store some data in there
    for (auto i = 0; i < 10; ++i)
    {
        auto vertices = generateVertices(i, (i + 5) * 20);
        auto indices = generateIndices(vertices);

        auto slot = store.allocateSlot(vertices.size(), indices.size());
        EXPECT_NE(slot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

        // Uploading the data should succeed
        EXPECT_NO_THROW(store.updateData(slot, vertices, indices));

        allocations.emplace_back(Allocation{ slot, vertices, indices });
    }

    // Verify all
    verifyAllAllocations(store, allocations);
    store.onFrameFinished();

    // Begin a new frame, the data in the new buffer should be up to date
    store.onFrameStart();
    verifyAllAllocations(store, allocations);
    store.onFrameFinished();

    auto dataUpdates = 0;
    auto subDataUpdates = 0;
    auto dataResizes = 0;
    auto allocationCount = 0;
    auto deallocationCount = 0;

    std::minstd_rand rand(17); // fixed seed

    // Run a few updates
    for (auto frame = 0; frame < 100; ++frame)
    {
        store.onFrameStart();

        // Verify all allocations at the start of every frame
        verifyAllAllocations(store, allocations);

        // Do something random with every allocation
        for (auto a = 0; a < allocations.size(); ++a)
        {
            auto& allocation = allocations[a];

            // Perform a random action
            switch (rand() % 7)
            {
            case 1: // updateSubData
            {
                subDataUpdates++;

                // Update 50% of the data
                auto newVertices = generateVertices(rand() % 9, allocation.vertices.size() >> 2);
                auto newIndices = generateIndices(newVertices);

                // Overwrite some of the data
                std::copy(newVertices.begin(), newVertices.end(), allocation.vertices.begin());
                std::copy(newIndices.begin(), newIndices.end(), allocation.indices.begin());

                store.updateSubData(allocation.slot, 0, newVertices, 0, newIndices);
                break;
            }

            case 2: // updateData
            {
                dataUpdates++;

                allocation.vertices = generateVertices(rand() % 9, allocation.vertices.size());
                allocation.indices = generateIndices(allocation.vertices);
                store.updateData(allocation.slot, allocation.vertices, allocation.indices);
                break;
            }

            case 3: // resize
            {
                dataResizes++;

                // Don't touch vertices below a minimum size
                if (allocation.vertices.size() < 10) break;

                // Allow 10% shrinking of the data
                auto newSize = allocation.vertices.size() - (rand() % (allocation.vertices.size() / 10));

                allocation.vertices.resize(newSize);
                allocation.indices = generateIndices(allocation.vertices);

                store.resizeData(allocation.slot, allocation.vertices.size(), allocation.indices.size());

                // after resize, we have to update the data too, unfortunately, otherwise the indices are out of bounds
                store.updateData(allocation.slot, allocation.vertices, allocation.indices);
                break;
            }

            case 4: // allocations
            {
                allocationCount++;

                auto vertices = generateVertices(rand() % 9, rand() % 100);
                auto indices = generateIndices(vertices);

                auto slot = store.allocateSlot(vertices.size(), indices.size());
                EXPECT_NE(slot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

                EXPECT_NO_THROW(store.updateData(slot, vertices, indices));
                allocations.emplace_back(Allocation{ slot, vertices, indices });
                break;
            }

            case 5: // dellocation
            {
                deallocationCount++;

                store.deallocateSlot(allocations[a].slot);
                allocations.erase(allocations.begin() + a);
                // We're going to skip one loop iteration, but that's not very important
                break;
            }
            } // switch
        }

        // Verify all allocations at the end of every frame
        verifyAllAllocations(store, allocations);

        store.onFrameFinished();
    }

    // One final check
    store.onFrameStart();
    verifyAllAllocations(store, allocations);
    store.onFrameFinished();

    EXPECT_GT(dataUpdates, 0) << "No data update operations performed";
    EXPECT_GT(subDataUpdates, 0) << "No sub data update operations performed";
    EXPECT_GT(dataResizes, 0) << "No resize operations performed";
    EXPECT_GT(allocationCount, 0) << "No allocation operations performed";
    EXPECT_GT(deallocationCount, 0) << "No deallocation operations performed";
}

TEST(GeometryStore, SyncObjectAcquisition)
{
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

    TestSyncObjectProvider::Instance().invocationCount = 0;

    for (int i = 0; i < 5; ++i)
    {
        store.onFrameStart();
        store.onFrameFinished();
    }

    EXPECT_EQ(TestSyncObjectProvider::Instance().invocationCount, 5) <<
        "GeometryStore should have performed 5 frame buffer switches";
}

TEST(GeometryStore, AllocateIndexRemap)
{
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

    // Allocate a slot to hold indexed vertices
    auto vertices = generateVertices(3, 15 * 20);
    auto indices = generateIndices(vertices);

    auto primarySlot = store.allocateSlot(vertices.size(), indices.size());
    EXPECT_NE(primarySlot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

    // Uploading the data should succeed
    EXPECT_NO_THROW(store.updateData(primarySlot, vertices, indices));

    auto secondarySlot = store.allocateIndexSlot(primarySlot, 20);
    EXPECT_NE(secondarySlot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

    // Deallocation through the regular method should succeed
    EXPECT_NO_THROW(store.deallocateSlot(secondarySlot));
}

TEST(GeometryStore, AllocateInvalidIndexRemap)
{
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

    // Allocate a slot to hold indexed vertices
    auto vertices = generateVertices(3, 15 * 20);
    auto indices = generateIndices(vertices);

    auto primarySlot = store.allocateSlot(vertices.size(), indices.size());
    EXPECT_NE(primarySlot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

    // This allocation is valid and will be a remap type
    auto secondarySlot = store.allocateIndexSlot(primarySlot, 20);
    EXPECT_NE(secondarySlot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

    // This call is not valid and should throw, since the secondary slot cannot be re-used
    EXPECT_THROW(store.allocateIndexSlot(secondarySlot, 10), std::logic_error);
}

TEST(GeometryStore, UpdateIndexRemapData)
{
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

    // Allocate a slot to hold indexed vertices
    auto vertices = generateVertices(3, 15 * 20);
    auto indices = generateIndices(vertices);

    auto primarySlot = store.allocateSlot(vertices.size(), indices.size());
    EXPECT_NE(primarySlot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";
    EXPECT_NO_THROW(store.updateData(primarySlot, vertices, indices));

    // Now allocate an index remapping slot, containing a straight, sequential set of indices 0..n-1
    std::vector<unsigned int> remap;
    remap.resize(vertices.size());
    std::iota(remap.begin(), remap.end(), 0);

    auto secondarySlot = store.allocateIndexSlot(primarySlot, remap.size());
    EXPECT_NE(secondarySlot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

    // Update the index data through updateData()
    EXPECT_NO_THROW(store.updateData(secondarySlot, {}, remap));

    // The render params should effectively point us the re-used vertices, in remapped order
    verifyAllocation(store, secondarySlot, vertices, remap);

    // Reverse the index order for testing the second way of uploading data
    std::reverse(remap.begin(), remap.end());
    EXPECT_NO_THROW(store.updateIndexData(secondarySlot, remap));

    verifyAllocation(store, secondarySlot, vertices, remap);

    // We expect an exception when trying to store vertex data
    EXPECT_THROW(store.updateData(secondarySlot, vertices, remap), std::logic_error);
}

TEST(GeometryStore, UpdateIndexRemapSubData)
{
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

    // Allocate a slot to hold indexed vertices
    auto vertices = generateVertices(3, 15 * 20);
    auto indices = generateIndices(vertices);

    auto primarySlot = store.allocateSlot(vertices.size(), indices.size());
    EXPECT_NE(primarySlot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";
    EXPECT_NO_THROW(store.updateData(primarySlot, vertices, indices));

    // Now allocate an index remapping slot, containing a straight, sequential set of indices 0..n-1
    std::vector<unsigned int> remap;
    remap.resize(vertices.size());
    std::iota(remap.begin(), remap.end(), 0);

    auto secondarySlot = store.allocateIndexSlot(primarySlot, remap.size());
    EXPECT_NE(secondarySlot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";

    // Upload the sequential set of indices
    EXPECT_NO_THROW(store.updateIndexData(secondarySlot, remap));
    verifyAllocation(store, secondarySlot, vertices, remap);

    // Generate a new set of indices, and make it half as large than the original remap
    std::vector<unsigned int> indexSubset;
    indexSubset.resize(remap.size() / 2);
    std::iota(indexSubset.begin(), indexSubset.end(), 0); // [0..N-1]
    std::reverse(indexSubset.begin(), indexSubset.end()); // make it [N-1...0]

    // Apply the subset to the local remap copy
    auto offset = indexSubset.size() / 4;
    std::copy(indexSubset.begin(), indexSubset.end(), remap.begin() + offset);

    // Apply the subset to the data in the store
    EXPECT_NO_THROW(store.updateIndexSubData(secondarySlot, offset, indexSubset));

    // The new subset should now be used when effective
    verifyAllocation(store, secondarySlot, vertices, remap);

    // We expect boundaries to be respected, this should be out of range
    EXPECT_THROW(store.updateIndexSubData(secondarySlot, remap.size() - 1, indexSubset), std::logic_error);
}

TEST(GeometryStore, ResizeIndexRemapData)
{
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

    // Allocate a slot to hold indexed vertices
    auto vertices = generateVertices(3, 15 * 20);
    auto indices = generateIndices(vertices);

    auto primarySlot = store.allocateSlot(vertices.size(), indices.size());
    EXPECT_NE(primarySlot, std::numeric_limits<render::IGeometryStore::Slot>::max()) << "Invalid slot";
    EXPECT_NO_THROW(store.updateData(primarySlot, vertices, indices));

    // Now allocate an index remapping slot, containing a straight, sequential set of indices 0..n-1
    std::vector<unsigned int> remap;
    remap.resize(vertices.size());
    std::iota(remap.begin(), remap.end(), 0);

    auto secondarySlot = store.allocateIndexSlot(primarySlot, remap.size());
    store.updateIndexData(secondarySlot, remap);

    verifyAllocation(store, secondarySlot, vertices, remap);

    // Cut off a few remap indices
    remap.resize(remap.size() - remap.size() / 3);
    EXPECT_NO_THROW(store.resizeData(secondarySlot, 0, remap.size()));

    // Verify this has taken effect
    verifyAllocation(store, secondarySlot, vertices, remap);

    // Cut off more indices, use the dedicated method this time
    remap.resize(remap.size() - remap.size() / 2);
    EXPECT_NO_THROW(store.resizeIndexData(secondarySlot, remap.size()));
    verifyAllocation(store, secondarySlot, vertices, remap);

    // We expect an exception if the index size is out of bounds
    EXPECT_THROW(store.resizeIndexData(secondarySlot, remap.size() * 20), std::logic_error);

    // And we cannot set the vertex size of an index remap slot
    EXPECT_THROW(store.resizeData(secondarySlot, 6, remap.size()), std::logic_error);
}

TEST(GeometryStore, RegularSlotBounds)
{
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

    // Allocate a slot to hold indexed vertices
    auto vertices = generateVertices(3, 15 * 20);
    auto indices = generateIndices(vertices);

    auto slot = store.allocateSlot(vertices.size(), indices.size());
    store.updateData(slot, vertices, indices);

    // This slot's indices are referencing all vertices, so the bounds
    // calculated should match the bounds of the entire vertex set.
    AABB localBounds;
    for (const auto& vertex : vertices)
    {
        const auto& v = vertex.vertex;
        localBounds.includePoint({ v.x(), v.y(), v.z() });
    }

    auto slotBounds = store.getBounds(slot);

    EXPECT_TRUE(math::isNear(slotBounds.getOrigin(), localBounds.getOrigin(), 0.01)) << "Bounds origin mismatch";
    EXPECT_TRUE(math::isNear(slotBounds.getExtents(), localBounds.getExtents(), 0.01)) << "Bounds extents mismatch";

    // Store a new set of indices in this slot that is just using every second vertex
    std::vector<unsigned int> newIndices;
    for (auto i = 0; i < vertices.size(); i += 2)
    {
        newIndices.push_back(i);
    }

    store.updateData(slot, vertices, newIndices);

    localBounds = AABB();
    for (auto index : newIndices)
    {
        const auto& v = vertices[index].vertex;
        localBounds.includePoint({ v.x(), v.y(), v.z() });
    }

    slotBounds = store.getBounds(slot);

    EXPECT_TRUE(math::isNear(slotBounds.getOrigin(), localBounds.getOrigin(), 0.01)) << "Bounds origin mismatch";
    EXPECT_TRUE(math::isNear(slotBounds.getExtents(), localBounds.getExtents(), 0.01)) << "Bounds extents mismatch";
}

TEST(GeometryStore, IndexRemappingSlotBounds)
{
    render::GeometryStore store(TestSyncObjectProvider::Instance(), _testBufferObjectProvider);

    // Allocate a slot to hold indexed vertices
    auto vertices = generateVertices(3, 15 * 20);
    auto indices = generateIndices(vertices);

    auto primarySlot = store.allocateSlot(vertices.size(), indices.size());
    store.updateData(primarySlot, vertices, indices);

    // Set up a remapping slot with a smaller set of indices referencing the primary slot vertices
    std::vector<unsigned int> newIndices(vertices.size() / 4);
    std::iota(newIndices.begin(), newIndices.end(), 0);

    auto indexSlot = store.allocateIndexSlot(primarySlot, newIndices.size());
    store.updateIndexData(indexSlot, newIndices);

    // Calculate the bounds of our offline mapping
    AABB localBounds;
    for (auto index : newIndices)
    {
        const auto& v = vertices[index].vertex;
        localBounds.includePoint({ v.x(), v.y(), v.z() });
    }

    // Query the bounds of this index slot, it should be the same
    auto slotBounds = store.getBounds(indexSlot);

    EXPECT_TRUE(math::isNear(slotBounds.getOrigin(), localBounds.getOrigin(), 0.01)) << "Bounds origin mismatch";
    EXPECT_TRUE(math::isNear(slotBounds.getExtents(), localBounds.getExtents(), 0.01)) << "Bounds extents mismatch";
}

}
