#include "gtest/gtest.h"
#include "render/ArbitraryMeshVertex.h"
#include "render/WindingRenderer.h"
#include "render/CompactWindingVertexBuffer.h"

namespace test
{

constexpr int SmallestWindingSize = 3;
constexpr int LargestWindingSize = 12;

using VertexBuffer = render::CompactWindingVertexBuffer<ArbitraryMeshVertex>;

inline ArbitraryMeshVertex createNthVertexOfWinding(int n, int id, std::size_t size)
{
    auto offset = static_cast<double>(n + size * id);

    return ArbitraryMeshVertex(
        { offset + 0.0, offset + 0.5, offset + 0.3 },
        { 0, 0, offset + 0.0 },
        { offset + 0.0, -offset + 0.0 }
    );
}

inline std::vector<ArbitraryMeshVertex> createWinding(int id, std::size_t size)
{
    std::vector<ArbitraryMeshVertex> winding;

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

}