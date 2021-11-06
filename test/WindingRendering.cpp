#include "gtest/gtest.h"
#include "render/ArbitraryMeshVertex.h"
#include "render/WindingRenderer.h"
#include "render/CompactWindingVertexBuffer.h"

namespace test
{

constexpr int SmallestWindingSize = 3;
constexpr int LargestWindingSize = 12;

using VertexBuffer = render::CompactWindingVertexBuffer<ArbitraryMeshVertex>;

inline std::vector<ArbitraryMeshVertex> createWinding(int id, int size)
{
    std::vector<ArbitraryMeshVertex> winding;

    for (int i = 0; i < size; ++i)
    {
        auto offset = static_cast<double>(i + size * id);
        winding.emplace_back(ArbitraryMeshVertex(
            { offset + 0.0, offset + 0.5, offset + 0.3 }, 
            { 0, 0, offset + 0.0 }, 
            { offset + 0.0, -offset + 0.0 }));
    }

    return winding;
}

inline void checkWindingIndices(const VertexBuffer& buffer, std::size_t slot)
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

            // All higher winding indices must have been adjusted
            auto remainingSlots = NumWindings - 1;
            for (auto slot = slotToRemove; slot < remainingSlots; ++slot)
            {
                checkWindingIndices(buffer, slot);
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
    auto numSlots = buffer.getVertices().size() / buffer.getWindingSize();

    EXPECT_THROW(buffer.removeWinding(numSlots), std::logic_error);
    EXPECT_THROW(buffer.removeWinding(numSlots + 1), std::logic_error);
    EXPECT_THROW(buffer.removeWinding(numSlots + 200), std::logic_error);
}

}
