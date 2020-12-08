#include "RadiantTest.h"

#include "imap.h"
#include "ipatch.h"
#include "patch/PatchIterators.h"

namespace test
{

using PatchIteratorTest = RadiantTest;

namespace
{

// Patch will be parallel to the XY plane, its vertex coordinates x = col*64 and y = row*64
inline IPatchNodePtr createWorldspawnPatch(std::size_t width, std::size_t height)
{
    auto world = GlobalMapModule().findOrInsertWorldspawn();

    auto sceneNode = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);
    auto patchNode = std::dynamic_pointer_cast<IPatchNode>(sceneNode);

    world->addChildNode(sceneNode);

    auto& patch = patchNode->getPatch();

    patch.setDims(width, height);

    for (auto row = 0; row < height; ++row)
    {
        for (auto col = 0; col < width; ++col)
        {
            patch.ctrlAt(row, col).vertex.set(col * 64, row * 64, 0);
            patch.ctrlAt(row, col).texcoord[0] = col * 1.0 / (width - 1.0);
            patch.ctrlAt(row, col).texcoord[1] = row * 1.0 / (height - 1.0);
        }
    }

    patch.controlPointsChanged();

    return patchNode;
}

}

TEST_F(PatchIteratorTest, IterateOverWholePatchColumnWise)
{
    auto patch = createWorldspawnPatch(3, 5);

    std::vector<Vector3> expectedValues;

    // Fill the vector with the expected values
    for (auto col = 0; col < patch->getPatch().getWidth(); ++col)
    {
        for (auto row = 0; row < patch->getPatch().getHeight(); ++row)
        {
            expectedValues.push_back(patch->getPatch().ctrlAt(row, col).vertex);
        }
    }

    patch::ColumnWisePatchIterator it(patch->getPatch());
    auto expected = expectedValues.begin();

    while (it.isValid())
    {
        EXPECT_EQ((it++)->vertex, *(expected++));
    }

    EXPECT_EQ(expected, expectedValues.end()); // assume no underflow
}

TEST_F(PatchIteratorTest, IterateOverWholePatchColumnWiseRowBackwards)
{
    auto patch = createWorldspawnPatch(3, 5);

    std::vector<Vector3> expectedValues;

    // Fill the vector with the expected values
    for (auto col = 0; col < patch->getPatch().getWidth(); ++col)
    {
        for (auto row = static_cast<int>(patch->getPatch().getHeight()) -1; row >= 0; --row)
        {
            expectedValues.push_back(patch->getPatch().ctrlAt(row, col).vertex);
        }
    }

    patch::ColumnWisePatchReverseIterator it(patch->getPatch());
    auto expected = expectedValues.begin();

    while (it.isValid())
    {
        EXPECT_EQ((it++)->vertex, *(expected++));
    }

    EXPECT_EQ(expected, expectedValues.end()); // assume no underflow
}

TEST_F(PatchIteratorTest, IterateOverWholePatchRowWise)
{
    auto patch = createWorldspawnPatch(3, 5);

    std::vector<Vector3> expectedValues;

    // Fill the vector with the expected values
    for (auto row = 0; row < patch->getPatch().getHeight(); ++row)
    {
        for (auto col = 0; col < patch->getPatch().getWidth(); ++col)
        {
            expectedValues.push_back(patch->getPatch().ctrlAt(row, col).vertex);
        }
    }

    patch::RowWisePatchIterator it(patch->getPatch());
    auto expected = expectedValues.begin();

    while (it.isValid())
    {
        EXPECT_EQ((it++)->vertex, *(expected++));
    }

    EXPECT_EQ(expected, expectedValues.end()); // assume no underflow
}

TEST_F(PatchIteratorTest, IterateOverWholePatchRowWiseColumnBackwards)
{
    auto patch = createWorldspawnPatch(3, 5);

    std::vector<Vector3> expectedValues;

    // Fill the vector with the expected values
    for (auto row = 0; row < patch->getPatch().getHeight(); ++row)
    {
        for (auto col = static_cast<int>(patch->getPatch().getWidth()) - 1; col >= 0; --col)
        {
            expectedValues.push_back(patch->getPatch().ctrlAt(row, col).vertex);
        }
    }

    patch::RowWisePatchReverseIterator it(patch->getPatch());
    auto expected = expectedValues.begin();

    while (it.isValid())
    {
        EXPECT_EQ((it++)->vertex, *(expected++));
    }

    EXPECT_EQ(expected, expectedValues.end()); // assume no underflow
}

void iterateOverPartialPatchColumnWise(IPatch& patch, std::size_t startCol, std::size_t endCol, bool rowBackwards)
{
    std::vector<Vector3> expectedValues;

    // Fill the vector with the expected values
    for (auto col = startCol; col <= endCol; ++col)
    {
        if (rowBackwards)
        {
            for (auto row = static_cast<int>(patch.getHeight()) - 1; row >= 0; --row)
            {
                expectedValues.push_back(patch.ctrlAt(row, col).vertex);
            }
        }
        else
        {
            for (auto row = 0; row < patch.getHeight(); ++row)
            {
                expectedValues.push_back(patch.ctrlAt(row, col).vertex);
            }
        }
    }

    auto it = !rowBackwards ?
        static_cast<patch::PatchControlIterator>(patch::ColumnWisePatchIterator(patch, startCol, endCol)) :
        patch::ColumnWisePatchReverseIterator(patch, startCol, endCol);

    auto expected = expectedValues.begin();

    while (it.isValid())
    {
        EXPECT_EQ((it++)->vertex, *(expected++));
    }

    EXPECT_EQ(expected, expectedValues.end()); // assume no underflow
}

TEST_F(PatchIteratorTest, IterateOverPartialPatchColumnWise)
{
    auto patch = createWorldspawnPatch(5, 7);

    // Try various start and end column configs
    iterateOverPartialPatchColumnWise(patch->getPatch(), 0, 4, false);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 0, 3, false);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 0, 0, false);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 0, 4, false);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 3, 4, false);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 1, 3, false);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 4, 4, false);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 3, 4, false);
}

TEST_F(PatchIteratorTest, IterateOverPartialPatchColumnWiseRowBackwards)
{
    auto patch = createWorldspawnPatch(5, 7);

    // Try various start and end column configs
    iterateOverPartialPatchColumnWise(patch->getPatch(), 0, 4, true);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 0, 3, true);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 0, 0, true);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 0, 4, true);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 3, 4, true);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 1, 3, true);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 4, 4, true);
    iterateOverPartialPatchColumnWise(patch->getPatch(), 3, 4, true);
}

void iterateOverPartialPatchRowWise(IPatch& patch, std::size_t startRow, std::size_t endRow, bool columnBackwards)
{
    std::vector<Vector3> expectedValues;

    // Fill the vector with the expected values
    for (auto row = startRow; row <= endRow; ++row)
    {
        if (columnBackwards)
        {
            for (auto col = static_cast<int>(patch.getWidth()) - 1; col >= 0; --col)
            {
                expectedValues.push_back(patch.ctrlAt(row, col).vertex);
            }
        }
        else
        {
            for (auto col = 0; col < patch.getWidth(); ++col)
            {
                expectedValues.push_back(patch.ctrlAt(row, col).vertex);
            }
        }
    }

    auto it = !columnBackwards ?
        static_cast<patch::PatchControlIterator>(patch::RowWisePatchIterator(patch, startRow, endRow)) :
        patch::RowWisePatchReverseIterator(patch, startRow, endRow);

    auto expected = expectedValues.begin();

    while (it.isValid())
    {
        EXPECT_EQ((it++)->vertex, *(expected++));
    }

    EXPECT_EQ(expected, expectedValues.end()); // assume no underflow
}

TEST_F(PatchIteratorTest, IterateOverPartialPatchRowWise)
{
    auto patch = createWorldspawnPatch(5, 7);

    // Try various start and end column configs
    iterateOverPartialPatchRowWise(patch->getPatch(), 0, 6, false);
    iterateOverPartialPatchRowWise(patch->getPatch(), 0, 3, false);
    iterateOverPartialPatchRowWise(patch->getPatch(), 0, 0, false);
    iterateOverPartialPatchRowWise(patch->getPatch(), 0, 4, false);
    iterateOverPartialPatchRowWise(patch->getPatch(), 3, 6, false);
    iterateOverPartialPatchRowWise(patch->getPatch(), 1, 3, false);
    iterateOverPartialPatchRowWise(patch->getPatch(), 6, 6, false);
    iterateOverPartialPatchRowWise(patch->getPatch(), 3, 4, false);
}

TEST_F(PatchIteratorTest, IterateOverPartialPatchRowWiseColumnBackwards)
{
    auto patch = createWorldspawnPatch(5, 7);

    // Try various start and end column configs
    iterateOverPartialPatchRowWise(patch->getPatch(), 0, 6, true);
    iterateOverPartialPatchRowWise(patch->getPatch(), 0, 3, true);
    iterateOverPartialPatchRowWise(patch->getPatch(), 0, 0, true);
    iterateOverPartialPatchRowWise(patch->getPatch(), 0, 4, true);
    iterateOverPartialPatchRowWise(patch->getPatch(), 3, 6, true);
    iterateOverPartialPatchRowWise(patch->getPatch(), 1, 3, true);
    iterateOverPartialPatchRowWise(patch->getPatch(), 6, 6, true);
    iterateOverPartialPatchRowWise(patch->getPatch(), 3, 4, true);
}

void iterateOverSingleColum(IPatch& patch, std::size_t colToTest)
{
    std::vector<Vector3> expectedValues;

    // Fill the vector with the expected values
    for (auto row = 0; row < patch.getHeight(); ++row)
    {
        expectedValues.push_back(patch.ctrlAt(row, colToTest).vertex);
    }

    patch::SinglePatchColumnIterator it(patch, colToTest);
    auto expected = expectedValues.begin();

    while (it.isValid())
    {
        EXPECT_EQ((it++)->vertex, *(expected++));
    }

    EXPECT_EQ(expected, expectedValues.end()); // assume no underflow
}

void iterateOverSingleColumReversely(IPatch& patch, std::size_t colToTest)
{
    std::vector<Vector3> expectedValues;

    // Fill the vector with the expected values
    for (auto row = static_cast<int>(patch.getHeight()) - 1; row >= 0; --row)
    {
        expectedValues.push_back(patch.ctrlAt(row, colToTest).vertex);
    }

    patch::SinglePatchColumnReverseIterator it(patch, colToTest);
    auto expected = expectedValues.begin();

    while (it.isValid())
    {
        EXPECT_EQ((it++)->vertex, *(expected++));
    }

    EXPECT_EQ(expected, expectedValues.end()); // assume no underflow
}

TEST_F(PatchIteratorTest, IterateOverSingleColumn)
{
    auto patch = createWorldspawnPatch(5, 7);

    iterateOverSingleColum(patch->getPatch(), 0);
    iterateOverSingleColum(patch->getPatch(), 1);
    iterateOverSingleColum(patch->getPatch(), 2);
    iterateOverSingleColum(patch->getPatch(), 3);
    iterateOverSingleColum(patch->getPatch(), 4);
}

TEST_F(PatchIteratorTest, IterateOverSingleColumnReversely)
{
    auto patch = createWorldspawnPatch(5, 7);

    iterateOverSingleColumReversely(patch->getPatch(), 0);
    iterateOverSingleColumReversely(patch->getPatch(), 1);
    iterateOverSingleColumReversely(patch->getPatch(), 2);
    iterateOverSingleColumReversely(patch->getPatch(), 3);
    iterateOverSingleColumReversely(patch->getPatch(), 4);
}

void iterateOverSingleRow(IPatch& patch, std::size_t rowToTest)
{
    std::vector<Vector3> expectedValues;

    // Fill the vector with the expected values
    for (auto col = 0; col < patch.getWidth(); ++col)
    {
        expectedValues.push_back(patch.ctrlAt(rowToTest, col).vertex);
    }

    patch::SinglePatchRowIterator it(patch, rowToTest);
    auto expected = expectedValues.begin();

    while (it.isValid())
    {
        EXPECT_EQ((it++)->vertex, *(expected++));
    }

    EXPECT_EQ(expected, expectedValues.end()); // assume no underflow
}

void iterateOverSingleRowReversely(IPatch& patch, std::size_t rowToTest)
{
    std::vector<Vector3> expectedValues;

    // Fill the vector with the expected values
    for (auto col = static_cast<int>(patch.getWidth()) - 1; col >= 0; --col)
    {
        expectedValues.push_back(patch.ctrlAt(rowToTest, col).vertex);
    }

    patch::SinglePatchRowReverseIterator it(patch, rowToTest);
    auto expected = expectedValues.begin();

    while (it.isValid())
    {
        EXPECT_EQ((it++)->vertex, *(expected++));
    }

    EXPECT_EQ(expected, expectedValues.end()); // assume no underflow
}

TEST_F(PatchIteratorTest, IterateOverSingleRow)
{
    auto patch = createWorldspawnPatch(5, 7);

    iterateOverSingleRow(patch->getPatch(), 0);
    iterateOverSingleRow(patch->getPatch(), 1);
    iterateOverSingleRow(patch->getPatch(), 2);
    iterateOverSingleRow(patch->getPatch(), 3);
    iterateOverSingleRow(patch->getPatch(), 4);
    iterateOverSingleRow(patch->getPatch(), 5);
    iterateOverSingleRow(patch->getPatch(), 6);
}

TEST_F(PatchIteratorTest, IterateOverSingleRowReversely)
{
    auto patch = createWorldspawnPatch(5, 7);

    iterateOverSingleRowReversely(patch->getPatch(), 0);
    iterateOverSingleRowReversely(patch->getPatch(), 1);
    iterateOverSingleRowReversely(patch->getPatch(), 2);
    iterateOverSingleRowReversely(patch->getPatch(), 3);
    iterateOverSingleRowReversely(patch->getPatch(), 4);
    iterateOverSingleRowReversely(patch->getPatch(), 5);
    iterateOverSingleRowReversely(patch->getPatch(), 6);
}

}
