#include "gtest/gtest.h"
#include "RadiantTest.h"
#include "scenelib.h"
#include "render/MeshVertex.h"
#include "render/CompactWindingVertexBuffer.h"
#include "render/WindingRenderer.h"
#include "render/GeometryStore.h"
#include "testutil/RenderUtils.h"
#include "testutil/TestBufferObjectProvider.h"
#include "testutil/TestObjectRenderer.h"
#include "testutil/TestSyncObjectProvider.h"
#include "algorithm/Entity.h"

namespace test
{

using WindingRendererTest = RadiantTest;

// Local test implementations of renderer interfaces
namespace
{
    TestBufferObjectProvider bufferObjectProvider;
    TestSyncObjectProvider syncObjectProvider;
    TestObjectRenderer testObjectRenderer;
}

constexpr int SmallestWindingSize = 3;
constexpr int LargestWindingSize = 12;

using VertexBuffer = render::CompactWindingVertexBuffer<MeshVertex>;

inline MeshVertex createNthVertexOfWinding(int n, int id, std::size_t size)
{
    auto offset = static_cast<double>(n + size * id);

    return MeshVertex(
        { offset + 0.0, offset + 0.5, offset + 0.3 },
        { 0, 0, offset + 0.0 },
        { offset + 0.0, -offset + 0.0 }
    );
}

inline std::vector<MeshVertex> createWinding(int id, std::size_t size)
{
    std::vector<MeshVertex> winding;

    for (int i = 0; i < size; ++i)
    {
        winding.emplace_back(createNthVertexOfWinding(i, id, size));
    }

    return winding;
}

inline void checkWindingIndices(const VertexBuffer& buffer, VertexBuffer::Slot slot)
{
    // Slot must be within range
    auto windingSize = buffer.getWindingSize();

    auto numWindingsInBuffer = buffer.getVertices().size() / windingSize;
    EXPECT_LT(slot, numWindingsInBuffer) << "Slot out of bounds";

    // Assume the indices are within bounds
    auto indexStart = buffer.getNumIndicesPerWinding() * slot;
    EXPECT_LE(indexStart + buffer.getNumIndicesPerWinding(), buffer.getIndices().size());

    // Check the indices, they must be referencing vertices in that slot
    for (auto i = indexStart; i < indexStart + buffer.getNumIndicesPerWinding(); ++i)
    {
        auto index = buffer.getIndices().at(i);
        EXPECT_GE(index, slot * windingSize) << "Winding index out of lower bounds";
        EXPECT_LT(index, (slot + 1) * windingSize) << "Winding index out of upper bounds";
    }
}

inline void checkWindingDataInSlot(const VertexBuffer& buffer, VertexBuffer::Slot slot, int expectedId)
{
    // Slot must be within range
    auto windingSize = buffer.getWindingSize();

    auto numWindingsInBuffer = buffer.getVertices().size() / windingSize;
    EXPECT_LT(slot, numWindingsInBuffer) << "Slot out of bounds";

    for (auto i = 0; i < windingSize; ++i)
    { 
        auto position = slot * windingSize + i;

        auto expectedVertex = createNthVertexOfWinding(i, expectedId, windingSize);
        auto vertex = buffer.getVertices().at(position);

        EXPECT_TRUE(math::isNear(vertex.vertex, expectedVertex.vertex, 0.01)) << "Vertex data mismatch";
        EXPECT_TRUE(math::isNear(vertex.texcoord, expectedVertex.texcoord, 0.01)) << "Texcoord data mismatch";
        EXPECT_TRUE(math::isNear(vertex.normal, expectedVertex.normal, 0.01)) << "Texcoord data mismatch";
    }
}

TEST(CompactWindingVertexBuffer, NumIndicesPerWinding)
{
    for (auto i = 0; i < 10; ++i)
    {
        VertexBuffer buffer(i);
        EXPECT_EQ(buffer.getWindingSize(), i);
        EXPECT_EQ(buffer.getNumIndicesPerWinding(), 3 * (buffer.getWindingSize() - 2));
    }
}

TEST(CompactWindingVertexBuffer, AddSingleWinding)
{
    for (auto size = SmallestWindingSize; size < LargestWindingSize; ++size)
    {
        VertexBuffer buffer(size);

        auto winding1 = createWinding(1, size);
        auto slot = buffer.pushWinding(winding1);

        EXPECT_EQ(slot, 0) << "Wrong slot assignment";
        EXPECT_EQ(buffer.getVertices().size(), size);
        EXPECT_EQ(buffer.getIndices().size(), buffer.getNumIndicesPerWinding());

        // Assume that the indices have been correctly calculated
        checkWindingIndices(buffer, slot);
        checkWindingDataInSlot(buffer, slot, 1); // ID is the same as in createWinding
    }
}

TEST(CompactWindingVertexBuffer, AddMultipleWindings)
{
    for (auto size = SmallestWindingSize; size < LargestWindingSize; ++size)
    {
        VertexBuffer buffer(size);

        // Add 10 windings to the buffer
        for (auto n = 1; n <= 10; ++n)
        {
            auto winding1 = createWinding(n, size);
            auto slot = buffer.pushWinding(winding1);

            EXPECT_EQ(slot, n-1) << "Unexpected slot assignment";
            EXPECT_EQ(buffer.getVertices().size(), n * size);
            EXPECT_EQ(buffer.getIndices().size(), n * buffer.getNumIndicesPerWinding());

            // Assume that the indices have been correctly calculated
            checkWindingIndices(buffer, slot);
            checkWindingDataInSlot(buffer, slot, n); // ID is the same as in createWinding
        }
    }
}

TEST(CompactWindingVertexBuffer, RemoveOneWinding)
{
    for (auto size = SmallestWindingSize; size < LargestWindingSize; ++size)
    {
        // We will work with a buffer containing 13 windings, 
        // the test will remove a single winding from every possible position
        constexpr auto NumWindings = 13;

        for (int slotToRemove = 0; slotToRemove < NumWindings; ++slotToRemove)
        {
            VertexBuffer buffer(size);

            // Add the desired number of windings to the buffer
            for (auto n = 1; n <= NumWindings; ++n)
            {
                buffer.pushWinding(createWinding(n, size));
            }

            // Remove a winding from the slot, this should move all
            // windings in greater slot numbers towards the left
            buffer.removeWinding(slotToRemove);

            // Check the resized vectors
            EXPECT_EQ(buffer.getVertices().size(), (NumWindings - 1) * buffer.getWindingSize()) << "Vertex array not resized";
            EXPECT_EQ(buffer.getIndices().size(), (NumWindings - 1) * buffer.getNumIndicesPerWinding()) << "Index array not resized";

            // All winding indices must still be correct
            auto remainingSlots = NumWindings - 1;
            for (auto slot = 0; slot < remainingSlots; ++slot)
            {
                checkWindingIndices(buffer, slot);

                // We expect the winding vertex data to be unchanged if the slot has not been touched
                auto id = slot + 1;
                checkWindingDataInSlot(buffer, slot, slot < slotToRemove ? id : id + 1);
            }
        }
    }
}

TEST(CompactWindingVertexBuffer, RemoveNonExistentSlot)
{
    VertexBuffer buffer(4);

    // Add a few windings to the buffer
    for (auto n = 1; n <= 20; ++n)
    {
        auto winding1 = createWinding(n, 4);
        buffer.pushWinding(winding1);
    }

    // Pass a non-existent slot number to removeWinding(), it should throw
    auto numSlots = static_cast<VertexBuffer::Slot>(buffer.getVertices().size() / buffer.getWindingSize());

    EXPECT_THROW(buffer.removeWinding(numSlots), std::logic_error);
    EXPECT_THROW(buffer.removeWinding(numSlots + 1), std::logic_error);
    EXPECT_THROW(buffer.removeWinding(numSlots + 200), std::logic_error);
}

TEST(CompactWindingVertexBuffer, ReplaceWinding)
{
    for (auto size = SmallestWindingSize; size < LargestWindingSize; ++size)
    {
        // We will work with a buffer containing 13 windings, 
        // the test will replace a single winding from every possible position
        constexpr auto NumWindings = 13;
        constexpr auto IdForReplacement = NumWindings * 2;

        for (int slotToReplace = 0; slotToReplace < NumWindings; ++slotToReplace)
        {
            VertexBuffer buffer(size);

            // Add the desired number of windings to the buffer
            for (auto n = 1; n <= NumWindings; ++n)
            {
                buffer.pushWinding(createWinding(n, size));
            }

            auto replacementWinding = createWinding(IdForReplacement, size);

            // Replace a winding in the desired slot
            buffer.replaceWinding(slotToReplace, replacementWinding);

            // Check the unchanged vector sizes
            EXPECT_EQ(buffer.getVertices().size(), NumWindings * buffer.getWindingSize()) << "Vertex array should remain unchanged";
            EXPECT_EQ(buffer.getIndices().size(), NumWindings * buffer.getNumIndicesPerWinding()) << "Index array should remain unchanged";

            // Check the slot data
            for (auto n = 1; n <= NumWindings; ++n)
            {
                auto slot = n - 1;

                checkWindingDataInSlot(buffer, slot, slot == slotToReplace ? IdForReplacement : n);
            }
        }
    }
}

TEST(CompactWindingVertexBuffer, RemoveMultipleWindings)
{
    for (auto size = SmallestWindingSize; size < LargestWindingSize; ++size)
    {
        // We will work with a buffer containing N windings, 
        // the test will remove 1 to N windings from every possible position (2^N - 1)
        constexpr auto NumWindings = 9;

        for (int combination = 1; combination <= (1 << NumWindings) - 1; ++combination)
        {
            VertexBuffer buffer(size);

            // Add the desired number of windings to the buffer
            for (auto n = 1; n <= NumWindings; ++n)
            {
                buffer.pushWinding(createWinding(n, size));
            }

            std::vector<VertexBuffer::Slot> slotsToRemove;

            // Translate the bit combination to a set of slots to remove
            for (int pos = 0; pos < NumWindings; ++pos)
            {
                if (combination & (1 << pos))
                {
                    slotsToRemove.push_back(pos);
                }
            }

            buffer.removeWindings(slotsToRemove);

            // The buffer should be smaller now
            auto remainingWindings = NumWindings - slotsToRemove.size();

            EXPECT_EQ(buffer.getVertices().size(), remainingWindings * buffer.getWindingSize()) <<
                "Winding vertex array has the wrong size after removal";
            EXPECT_EQ(buffer.getIndices().size(), remainingWindings * buffer.getNumIndicesPerWinding()) <<
                "Winding index array has the wrong size after removal";

            // Check the winding data, the ones we removed should be missing now
            for (int i = 0, slot = 0; i < NumWindings; ++i)
            {
                if (std::find(slotsToRemove.begin(), slotsToRemove.end(), i) != slotsToRemove.end())
                {
                    continue; // this id got removed, skip it
                }

                checkWindingIndices(buffer, slot);
                checkWindingDataInSlot(buffer, slot, i + 1);
                ++slot;
            }
        }
    }
}

TEST(CompactWindingVertexBuffer, TriangleIndexerSize3) // Winding size == 3
{
    render::CompactWindingVertexBuffer<MeshVertex, render::WindingIndexer_Triangles> buffer(3);

    EXPECT_EQ(buffer.getNumIndicesPerWinding(), 3);

    // Generate winding indices and check the result
    std::vector<unsigned int> indices;
    render::WindingIndexer_Triangles::GenerateAndAssignIndices(std::back_inserter(indices), buffer.getWindingSize(), 80);

    EXPECT_EQ(indices.size(), buffer.getNumIndicesPerWinding()) << "Wrong number of indices generated";

    EXPECT_EQ(indices[0], 80) << "Index 0 mismatch";
    EXPECT_EQ(indices[1], 81) << "Index 1 mismatch";
    EXPECT_EQ(indices[2], 82) << "Index 2 mismatch";
}

TEST(CompactWindingVertexBuffer, TriangleIndexerSize4) // Winding size == 4
{
    render::CompactWindingVertexBuffer<MeshVertex, render::WindingIndexer_Triangles> buffer(4);

    EXPECT_EQ(buffer.getNumIndicesPerWinding(), 6);

    // Generate winding indices and check the result
    std::vector<unsigned int> indices;
    render::WindingIndexer_Triangles::GenerateAndAssignIndices(std::back_inserter(indices), buffer.getWindingSize(), 80);

    EXPECT_EQ(indices.size(), buffer.getNumIndicesPerWinding()) << "Wrong number of indices generated";

    EXPECT_EQ(indices[0], 80) << "Index 0 mismatch";
    EXPECT_EQ(indices[1], 82) << "Index 1 mismatch";
    EXPECT_EQ(indices[2], 83) << "Index 2 mismatch";

    EXPECT_EQ(indices[3], 80) << "Index 3 mismatch";
    EXPECT_EQ(indices[4], 81) << "Index 4 mismatch";
    EXPECT_EQ(indices[5], 82) << "Index 5 mismatch";
}

TEST(CompactWindingVertexBuffer, TriangleIndexerSize5) // Winding size == 5
{
    render::CompactWindingVertexBuffer<MeshVertex, render::WindingIndexer_Triangles> buffer(5);

    EXPECT_EQ(buffer.getNumIndicesPerWinding(), 9);

    // Generate winding indices and check the result
    std::vector<unsigned int> indices;
    render::WindingIndexer_Triangles::GenerateAndAssignIndices(std::back_inserter(indices), buffer.getWindingSize(), 80);

    EXPECT_EQ(indices.size(), buffer.getNumIndicesPerWinding()) << "Wrong number of indices generated";

    EXPECT_EQ(indices[0], 80) << "Index 0 mismatch";
    EXPECT_EQ(indices[1], 83) << "Index 1 mismatch";
    EXPECT_EQ(indices[2], 84) << "Index 2 mismatch";

    EXPECT_EQ(indices[3], 80) << "Index 3 mismatch";
    EXPECT_EQ(indices[4], 82) << "Index 4 mismatch";
    EXPECT_EQ(indices[5], 83) << "Index 5 mismatch";

    EXPECT_EQ(indices[6], 80) << "Index 6 mismatch";
    EXPECT_EQ(indices[7], 81) << "Index 7 mismatch";
    EXPECT_EQ(indices[8], 82) << "Index 8 mismatch";
}

TEST(CompactWindingVertexBuffer, LineIndexerSize3) // Winding size == 3
{
    render::CompactWindingVertexBuffer<MeshVertex, render::WindingIndexer_Lines> buffer(3);

    EXPECT_EQ(buffer.getNumIndicesPerWinding(), 6);

    // Generate winding indices and check the result
    std::vector<unsigned int> indices;
    render::WindingIndexer_Lines::GenerateAndAssignIndices(std::back_inserter(indices), buffer.getWindingSize(), 80);

    EXPECT_EQ(indices.size(), buffer.getNumIndicesPerWinding()) << "Wrong number of indices generated";

    EXPECT_EQ(indices[0], 80) << "Index 0 mismatch";
    EXPECT_EQ(indices[1], 81) << "Index 1 mismatch";

    EXPECT_EQ(indices[2], 81) << "Index 2 mismatch";
    EXPECT_EQ(indices[3], 82) << "Index 3 mismatch";

    EXPECT_EQ(indices[4], 82) << "Index 4 mismatch";
    EXPECT_EQ(indices[5], 80) << "Index 5 mismatch";
}

TEST(CompactWindingVertexBuffer, LineIndexerSize4) // Winding size == 4
{
    render::CompactWindingVertexBuffer<MeshVertex, render::WindingIndexer_Lines> buffer(4);

    EXPECT_EQ(buffer.getNumIndicesPerWinding(), 8);

    // Generate winding indices and check the result
    std::vector<unsigned int> indices;
    render::WindingIndexer_Lines::GenerateAndAssignIndices(std::back_inserter(indices), buffer.getWindingSize(), 80);

    EXPECT_EQ(indices.size(), buffer.getNumIndicesPerWinding()) << "Wrong number of indices generated";

    EXPECT_EQ(indices[0], 80) << "Index 0 mismatch";
    EXPECT_EQ(indices[1], 81) << "Index 1 mismatch";

    EXPECT_EQ(indices[2], 81) << "Index 2 mismatch";
    EXPECT_EQ(indices[3], 82) << "Index 3 mismatch";

    EXPECT_EQ(indices[4], 82) << "Index 4 mismatch";
    EXPECT_EQ(indices[5], 83) << "Index 5 mismatch";

    EXPECT_EQ(indices[6], 83) << "Index 6 mismatch";
    EXPECT_EQ(indices[7], 80) << "Index 7 mismatch";
}

TEST(CompactWindingVertexBuffer, LineIndexerSize5) // Winding size == 5
{
    render::CompactWindingVertexBuffer<MeshVertex, render::WindingIndexer_Lines> buffer(5);

    EXPECT_EQ(buffer.getNumIndicesPerWinding(), 10);

    // Generate winding indices and check the result
    std::vector<unsigned int> indices;
    render::WindingIndexer_Lines::GenerateAndAssignIndices(std::back_inserter(indices), buffer.getWindingSize(), 80);

    EXPECT_EQ(indices.size(), buffer.getNumIndicesPerWinding()) << "Wrong number of indices generated";

    EXPECT_EQ(indices[0], 80) << "Index 0 mismatch";
    EXPECT_EQ(indices[1], 81) << "Index 1 mismatch";

    EXPECT_EQ(indices[2], 81) << "Index 2 mismatch";
    EXPECT_EQ(indices[3], 82) << "Index 3 mismatch";

    EXPECT_EQ(indices[4], 82) << "Index 4 mismatch";
    EXPECT_EQ(indices[5], 83) << "Index 5 mismatch";

    EXPECT_EQ(indices[6], 83) << "Index 6 mismatch";
    EXPECT_EQ(indices[7], 84) << "Index 7 mismatch";

    EXPECT_EQ(indices[8], 84) << "Index 8 mismatch";
    EXPECT_EQ(indices[9], 80) << "Index 9 mismatch";
}

TEST(CompactWindingVertexBuffer, PolygonIndexerSize5) // Winding size == 5
{
    render::CompactWindingVertexBuffer<MeshVertex, render::WindingIndexer_Polygon> buffer(5);

    EXPECT_EQ(buffer.getNumIndicesPerWinding(), 5);

    // Generate winding indices and check the result
    std::vector<unsigned int> indices;
    render::WindingIndexer_Polygon::GenerateAndAssignIndices(std::back_inserter(indices), buffer.getWindingSize(), 80);

    EXPECT_EQ(indices.size(), buffer.getNumIndicesPerWinding()) << "Wrong number of indices generated";

    EXPECT_EQ(indices[0], 80) << "Index 0 mismatch";
    EXPECT_EQ(indices[1], 81) << "Index 1 mismatch";
    EXPECT_EQ(indices[2], 82) << "Index 2 mismatch";
    EXPECT_EQ(indices[3], 83) << "Index 3 mismatch";
    EXPECT_EQ(indices[4], 84) << "Index 4 mismatch";
}

inline std::size_t getNumEntitySurfaces(IRenderEntity* entity)
{
    std::size_t count = 0;

    entity->foreachRenderable([&](const render::IRenderableObject::Ptr& object, Shader* shader)
    {
        ++count;
    });

    return count;
}

inline render::IRenderableObject::Ptr getNthEntitySurface(IRenderEntity* entity, std::size_t index)
{
    std::size_t count = 0;
    render::IRenderableObject::Ptr foundObject;

    entity->foreachRenderable([&](const render::IRenderableObject::Ptr& object, Shader* _)
    {
        if (count++ == index)
        {
            foundObject = object;
        }
    });

    return foundObject;
}

TEST_F(WindingRendererTest, EntitySurfaceCreation)
{
    render::GeometryStore geometryStore(syncObjectProvider, bufferObjectProvider);
    auto shader = GlobalRenderSystem().capture("textures/common/caulk");
    auto entity = algorithm::createEntityByClassName("func_static");
    scene::addNodeToContainer(entity, GlobalMapModule().getRoot());

    // Create a  new triangle WindingRenderer
    render::WindingRenderer<render::WindingIndexer_Triangles> renderer(geometryStore, testObjectRenderer, shader.get());

    // Test windings of various sizes
    std::vector<render::RenderVertex> winding4 = generateVertices(1, 4);
    std::vector<render::RenderVertex> winding3 = generateVertices(1, 3);

    EXPECT_EQ(getNumEntitySurfaces(entity.get()), 0) << "Entity should start with no surfaces";

    auto winding4Slot = renderer.addWinding(winding4, entity.get());
    EXPECT_EQ(getNumEntitySurfaces(entity.get()), 1) << "Entity should have 1 surface now";

    auto winding3Slot = renderer.addWinding(winding3, entity.get());
    EXPECT_EQ(getNumEntitySurfaces(entity.get()), 2) << "Entity should have 2 surfaces now";

    renderer.removeWinding(winding4Slot);
    EXPECT_EQ(getNumEntitySurfaces(entity.get()), 1) << "Entity should have 1 surface now";

    renderer.removeWinding(winding3Slot);
    EXPECT_EQ(getNumEntitySurfaces(entity.get()), 0) << "Entity should be empty again";
}

TEST_F(WindingRendererTest, EntitySurfacesRemovedInDestructor)
{
    render::GeometryStore geometryStore(syncObjectProvider, bufferObjectProvider);
    auto shader = GlobalRenderSystem().capture("textures/common/caulk");
    auto entity = algorithm::createEntityByClassName("func_static");
    scene::addNodeToContainer(entity, GlobalMapModule().getRoot());

    // Create a  new triangle WindingRenderer
    auto renderer = std::make_unique<render::WindingRenderer<render::WindingIndexer_Triangles>>(geometryStore, testObjectRenderer, shader.get());

    // Test windings of various sizes
    std::vector<render::RenderVertex> winding4 = generateVertices(1, 4);
    std::vector<render::RenderVertex> winding3 = generateVertices(1, 3);

    renderer->addWinding(winding4, entity.get());
    renderer->addWinding(winding3, entity.get());
    EXPECT_EQ(getNumEntitySurfaces(entity.get()), 2) << "Entity should have 2 surfaces now";

    // All entity surfaces should be removed in the WindingRenderer's destructor
    renderer.reset();
    EXPECT_EQ(getNumEntitySurfaces(entity.get()), 0) << "Entity should be empty again";
}

// Checks that the given vertex slot contains the given set of vertices
void verifyVertexSlot(render::IGeometryStore& store, render::IGeometryStore::Slot slot, 
    const std::vector<render::RenderVertex>& vertices)
{
    auto renderParms = store.getRenderParameters(slot);

    auto expectedVertex = vertices.begin();

    for (auto vertex = renderParms.clientBufferStart + renderParms.firstVertex; 
         expectedVertex != vertices.end();
         ++vertex, ++expectedVertex)
    {
        EXPECT_TRUE(math::isNear(vertex->vertex, expectedVertex->vertex, 0.01)) << "Vertex data mismatch";
        EXPECT_TRUE(math::isNear(vertex->texcoord, expectedVertex->texcoord, 0.01)) << "Texcoord data mismatch";
        EXPECT_TRUE(math::isNear(vertex->normal, expectedVertex->normal, 0.01)) << "Normal data mismatch";
    }
}

// #5963: Entity surface IndexRemap slots not updated after moving the CompactWindingVertexBuffer to a different vertex geometry slot
TEST_F(WindingRendererTest, IndexRemapsUpdatedAfterReallocation)
{
    render::GeometryStore geometryStore(syncObjectProvider, bufferObjectProvider);
    auto shader = GlobalRenderSystem().capture("textures/common/caulk");
    auto entity = algorithm::createEntityByClassName("func_static");
    auto entity2 = algorithm::createEntityByClassName("func_static");
    scene::addNodeToContainer(entity, GlobalMapModule().getRoot());
    scene::addNodeToContainer(entity2, GlobalMapModule().getRoot());

    // Create a  new triangle WindingRenderer
    auto renderer = std::make_unique<render::WindingRenderer<render::WindingIndexer_Triangles>>(geometryStore, testObjectRenderer, shader.get());

    // Generate a couple of windings
    std::vector<render::RenderVertex> entity1Windings;
    for (int i = 0; i < 3; ++i)
    {
        std::vector<render::RenderVertex> winding4 = generateVertices(i, 4);
        renderer->addWinding(winding4, entity.get());
        std::copy(winding4.begin(), winding4.end(), std::back_inserter(entity1Windings));
    }

    // Make sure the windings are stored in the geometry store
    renderer->prepareForRendering();

    // Acquire a few unrelated blocks in the geometry store to simulate other objects allocating memory
    geometryStore.allocateSlot(15, 24);
    geometryStore.allocateSlot(50, 60);
    geometryStore.allocateSlot(5, 12);
    geometryStore.allocateSlot(100, 150);

    EXPECT_EQ(getNumEntitySurfaces(entity.get()), 1) << "Entity should have 1 surface now";

    auto surfaceEntity1 = getNthEntitySurface(entity.get(), 0);
    EXPECT_TRUE(surfaceEntity1) << "Entity Surface not found";

    // Verify the winding geometry in the given slot
    verifyVertexSlot(geometryStore, surfaceEntity1->getStorageLocation(), entity1Windings);

    // Remember the offset of the first vertex
    auto firstVertexOffsetBeforeMove = geometryStore.getRenderParameters(surfaceEntity1->getStorageLocation()).firstVertex;

    // Now add a few more windings for the second entity, this should invalidate all
    // the entity surfaces of the renderer (it failed to do so before #5963).
    // The WindingRenderer will have to reallocate its geometry slot, so the vertices
    // will be located at a different memory location. Since we added unrelated blocks, we force the
    // larger allocation to be moved to the end of the geometry store
    std::vector<render::RenderVertex> entity2Windings;
    for (int i = 0; i < 5; ++i)
    {
        std::vector<render::RenderVertex> winding4 = generateVertices(i + 10, 4);
        renderer->addWinding(winding4, entity2.get());
        std::copy(winding4.begin(), winding4.end(), std::back_inserter(entity2Windings));
    }

    // Force another geometry store update
    renderer->prepareForRendering();

    // Get the surface again
    surfaceEntity1 = getNthEntitySurface(entity.get(), 0);
    auto surfaceEntity2 = getNthEntitySurface(entity2.get(), 0);

    // Now check the connection of the IndexRemap slots of entity 1's surface,
    // it should be correctly referring to the moved vertex slot
    verifyVertexSlot(geometryStore, surfaceEntity1->getStorageLocation(), entity1Windings);

    // The first vertex' offset of entity 1's surface ought to be different now
    auto firstVertexOffsetAfterMove = geometryStore.getRenderParameters(surfaceEntity1->getStorageLocation()).firstVertex;
    EXPECT_NE(firstVertexOffsetAfterMove, firstVertexOffsetBeforeMove) << "First vertex offset didn't change, this is very suspicious";

    // Compare the vertex offset of entity1's surface to that of entity2, they should match
    auto entity2VertexOffset = geometryStore.getRenderParameters(surfaceEntity2->getStorageLocation()).firstVertex;
    EXPECT_EQ(entity2VertexOffset, firstVertexOffsetAfterMove) << "First vertex offset of both entities should match";
}

}
