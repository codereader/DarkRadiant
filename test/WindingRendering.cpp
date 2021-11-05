#include "gtest/gtest.h"
#include "render/ArbitraryMeshVertex.h"
#include "render/WindingRenderer.h"
#include "render/CompactWindingVertexBuffer.h"

namespace test
{

using VertexBuffer = render::CompactWindingVertexBuffer<ArbitraryMeshVertex>;

inline std::vector<ArbitraryMeshVertex> createWinding(int id, int size)
{
    std::vector<ArbitraryMeshVertex> winding;

    for (int i = 0; i < size; ++i)
    {
        winding.emplace_back(ArbitraryMeshVertex({ id + 0.0, id + 0.5, id + 0.3 }, { 0, 0, id + 0.0 }, { id + 0.0, -id + 0.0 }));
    }

    return winding;
}

inline void checkWindingIndices(const VertexBuffer& buffer, std::size_t slot)
{
    // Slot must be within range
    auto windingSize = buffer.getWindingSize();

    EXPECT_LT(slot, buffer.getVertices().size() / windingSize);

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
    auto winding1 = createWinding(1, 4);

    VertexBuffer buffer(4);

    auto slot = buffer.pushWinding(winding1);

    EXPECT_EQ(slot, 0) << "Wrong slot assignment";
    EXPECT_EQ(buffer.getVertices().size(), 4);
    EXPECT_EQ(buffer.getIndices().size(), buffer.getNumIndicesPerWinding());

    // Assume that the indices have been correctly calculated
    checkWindingIndices(buffer, slot);
}

}
