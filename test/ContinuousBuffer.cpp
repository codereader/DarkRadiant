#include "gtest/gtest.h"

#include "render/ContinuousBuffer.h"
#include "testutil/TestBufferObjectProvider.h"

namespace test
{

namespace
{

TestBufferObjectProvider _testBufferObjectProvider;

}

template<typename T>
bool checkData(render::ContinuousBuffer<T>& buffer, typename render::ContinuousBuffer<T>::Handle handle, const std::vector<T>& data)
{
    auto current = buffer.getBufferStart() + buffer.getOffset(handle);

    bool result = true;

    for (auto i = 0; i < data.size(); ++i, ++current)
    {
        EXPECT_EQ(data.at(i), *current) << "Buffer data mismatch at index " << i;
        result &= data.at(i) == *current;
    }

    return result;
}

template<typename T>
bool checkContinuousData(render::ContinuousBuffer<T>& buffer, typename render::ContinuousBuffer<T>::Handle handle, 
    const std::initializer_list<std::vector<T>>& dataVectors)
{
    // Create a continuous buffer
    std::vector<T> fullData;

    for (const auto& data : dataVectors)
    {
        fullData.insert(fullData.end(), data.begin(), data.end());
    }

    auto result = checkData(buffer, handle, fullData);

    EXPECT_TRUE(result) << "Data not continuously stored";

    return result;
}

template<typename T>
bool checkDataInBufferObject(render::ContinuousBuffer<T>& buffer, typename render::ContinuousBuffer<T>::Handle handle, 
    TestBufferObject& bufferObject, const std::vector<T>& data)
{
    // Data start in ContinuousBuffer
    auto current = buffer.getBufferStart() + buffer.getOffset(handle);

    // Data start in the BufferObject
    auto dataInBuffer = reinterpret_cast<const T*>(bufferObject.buffer.data()) + buffer.getOffset(handle);

    bool result = true;

    for (auto i = 0; i < data.size(); ++i)
    {
        EXPECT_EQ(dataInBuffer[i], current[i]) << "Buffer data mismatch at index " << i;
        result &= dataInBuffer[i] == current[i];
    }

    return result;
}

TEST(ContinuousBufferTest, InitialSize)
{
    // Construct a buffer of a certain initial size
    render::ContinuousBuffer<int> buffer(2 << 16);
    EXPECT_NE(buffer.getBufferStart(), nullptr);
}

TEST(ContinuousBufferTest, ZeroInitialSize)
{
    // It's ok to construct a buffer of empty size
    render::ContinuousBuffer<int> buffer(0);
    EXPECT_NE(buffer.getBufferStart(), nullptr);
}

TEST(ContinuousBufferTest, AllocateAndDeallocate)
{
    auto sixteen = std::vector<int>({ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 });

    // Allocate a buffer of size 24
    render::ContinuousBuffer<int> buffer(24);
    
    auto handle = buffer.allocate(sixteen.size());
    EXPECT_EQ(buffer.getNumUsedElements(handle), 0) << "Allocated storage should be unused";
    buffer.setData(handle, sixteen);
    EXPECT_EQ(buffer.getNumUsedElements(handle), sixteen.size()) << "Used element count should be 16 now";

    EXPECT_EQ(buffer.getOffset(handle), 0) << "Data should be located at offset 0";
    EXPECT_TRUE(checkData(buffer, handle, sixteen));

    buffer.deallocate(handle);

    // Re-allocate
    handle = buffer.allocate(sixteen.size());
    EXPECT_EQ(buffer.getNumUsedElements(handle), 0) << "Allocated storage should be unused";
    buffer.setData(handle, sixteen);
    EXPECT_EQ(buffer.getNumUsedElements(handle), sixteen.size()) << "Used element count should be 16 now";

    EXPECT_EQ(buffer.getOffset(handle), 0) << "Data should be located at offset 0";
    EXPECT_TRUE(checkData(buffer, handle, sixteen));
}

TEST(ContinuousBufferTest, ReplaceData)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto eightAlt = std::vector<int>({ 8,9,10,11,12,13,14,15 });

    render::ContinuousBuffer<int> buffer(24);

    auto handle = buffer.allocate(eight.size());
    buffer.setData(handle, eight);

    EXPECT_TRUE(checkData(buffer, handle, eight));
    EXPECT_EQ(buffer.getSize(handle), eight.size());
    EXPECT_EQ(buffer.getNumUsedElements(handle), eight.size());

    buffer.setData(handle, eightAlt);
    EXPECT_TRUE(checkData(buffer, handle, eightAlt));
    EXPECT_EQ(buffer.getNumUsedElements(handle), eightAlt.size());
}

TEST(ContinuousBufferTest, ReplaceDataPartially)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto ten = std::vector<int>({ 8,9,10,11,12,13,14,15,16,17 });

    render::ContinuousBuffer<int> buffer(24);

    auto allocatedSize = eight.size() + 6; // 6 extra elements
    auto handle = buffer.allocate(allocatedSize);
    buffer.setData(handle, eight);

    EXPECT_TRUE(checkData(buffer, handle, eight));
    EXPECT_EQ(buffer.getSize(handle), allocatedSize);
    EXPECT_EQ(buffer.getNumUsedElements(handle), eight.size());

    buffer.setData(handle, ten);
    EXPECT_TRUE(checkData(buffer, handle, ten));
    EXPECT_EQ(buffer.getSize(handle), allocatedSize);
    EXPECT_EQ(buffer.getNumUsedElements(handle), ten.size());
}

TEST(ContinuousBufferTest, ReplaceDataOverflow)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });

    render::ContinuousBuffer<int> buffer(24);

    auto handle = buffer.allocate(eight.size() - 3); // too little space

    EXPECT_THROW(buffer.setData(handle, eight), std::logic_error);
}

TEST(ContinuousBufferTest, ReplaceSubData)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto five = std::vector<int>({ 8,9,10,11,12 });

    render::ContinuousBuffer<int> buffer(24);

    auto handle = buffer.allocate(eight.size());
    buffer.setData(handle, eight);

    EXPECT_TRUE(checkData(buffer, handle, eight));

    EXPECT_EQ(buffer.getSize(handle), eight.size());
    EXPECT_EQ(buffer.getNumUsedElements(handle), eight.size());

    // Update the data portion at various offsets
    buffer.setSubData(handle, 0, five);
    EXPECT_TRUE(checkData(buffer, handle, { 8,9,10,11,12, 5,6,7 }));
    EXPECT_EQ(buffer.getNumUsedElements(handle), 8);

    buffer.setSubData(handle, 1, five);
    EXPECT_TRUE(checkData(buffer, handle, { 8, 8,9,10,11,12, 6,7 }));
    EXPECT_EQ(buffer.getNumUsedElements(handle), 8);

    buffer.setSubData(handle, 2, five);
    EXPECT_TRUE(checkData(buffer, handle, { 8,8, 8,9,10,11,12, 7 }));
    EXPECT_EQ(buffer.getNumUsedElements(handle), 8);

    buffer.setSubData(handle, 3, five);
    EXPECT_TRUE(checkData(buffer, handle, { 8,8,8, 8,9,10,11,12 }));
    EXPECT_EQ(buffer.getNumUsedElements(handle), 8);
}

// In a chunk of memory that is only partially used, calling setSubData
// can increase that amount of used elements
TEST(ContinuousBufferTest, ReplaceSubDataIncreasesUsedSize)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto five = std::vector<int>({ 8,9,10,11,12 });

    render::ContinuousBuffer<int> buffer(24);

    // Allocate 6 more elements than needed
    auto allocationSize = eight.size() + 6;
    auto handle = buffer.allocate(allocationSize);
    buffer.setData(handle, eight);

    EXPECT_TRUE(checkData(buffer, handle, eight));

    EXPECT_EQ(buffer.getSize(handle), allocationSize);
    EXPECT_EQ(buffer.getNumUsedElements(handle), eight.size());

    // Update the data portion at an offset that increases the fill rate
    buffer.setSubData(handle, 4, five);
    EXPECT_TRUE(checkData(buffer, handle, { 0,1,2,3,8,9,10,11,12 }));
    EXPECT_EQ(buffer.getSize(handle), allocationSize);
    EXPECT_EQ(buffer.getNumUsedElements(handle), 9) << "Should have increased use count to 9";
}

// Tests that subdata is still respecting the allocation bounds
TEST(ContinuousBufferTest, ReplaceSubDataOverflow)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto five = std::vector<int>({ 8,9,10,11,12 });

    render::ContinuousBuffer<int> buffer(24);

    auto handle = buffer.allocate(eight.size());
    buffer.setData(handle, eight);

    EXPECT_TRUE(checkData(buffer, handle, eight));

    // Update the data portion at an invalid offset, exceeding the allocated slot size
    EXPECT_THROW(buffer.setSubData(handle, 4, five), std::logic_error);
}

TEST(ContinuousBufferTest, ResizeData)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });

    render::ContinuousBuffer<int> buffer(24);

    auto handle = buffer.allocate(eight.size());
    buffer.setData(handle, eight);

    EXPECT_EQ(buffer.getNumUsedElements(handle), eight.size());

    buffer.resizeData(handle, 5);
    EXPECT_TRUE(checkData(buffer, handle, eight)) << "Data should not have changed";
    EXPECT_EQ(buffer.getNumUsedElements(handle), 5) << "Used Element Count should be 5 now";
}

TEST(ContinuousBufferTest, ResizeDataOverflow)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });

    render::ContinuousBuffer<int> buffer(24);

    auto handle = buffer.allocate(eight.size());
    buffer.setData(handle, eight);
    
    EXPECT_EQ(buffer.getNumUsedElements(handle), eight.size());

    EXPECT_THROW(buffer.resizeData(handle, eight.size() + 1), std::logic_error);
}

TEST(ContinuousBufferTest, BufferGrowth)
{
    auto sixteen = std::vector<int>({ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 });
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });

    // Allocate a buffer of size 18
    render::ContinuousBuffer<int> buffer(18);

    // Allocate three slots, the size of which will exceed the initial size
    auto handle1 = buffer.allocate(sixteen.size());
    buffer.setData(handle1, sixteen);
    EXPECT_TRUE(checkData(buffer, handle1, sixteen)) << "First data batch not stored";

    // Push another 8, triggering buffer growth
    auto handle2 = buffer.allocate(eight.size());
    buffer.setData(handle2, eight);

    EXPECT_TRUE(checkData(buffer, handle1, sixteen)) << "First data batch should still be intact";
    EXPECT_TRUE(checkData(buffer, handle2, eight)) << "Second data batch not stored correctly";

    // Another one
    auto handle3 = buffer.allocate(eight.size());
    buffer.setData(handle3, eight);

    EXPECT_TRUE(checkData(buffer, handle1, sixteen)) << "First data batch should still be intact";
    EXPECT_TRUE(checkData(buffer, handle2, eight)) << "Second data batch should still be intact";
    EXPECT_TRUE(checkData(buffer, handle3, eight)) << "Third data batch not stored correctly";

    // Data should be continuously stored
    EXPECT_TRUE(checkContinuousData(buffer, handle1, { sixteen, eight, eight }));

    buffer.deallocate(handle1);
    buffer.deallocate(handle2);
    buffer.deallocate(handle3);
}

TEST(ContinuousBufferTest, BlockReuse)
{
    auto sixteen = std::vector<int>({ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 });
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto eightFrom6 = std::vector<int>({ 6,7,8,9,10,11,12,13 });

    // Allocate a buffer of size 18
    render::ContinuousBuffer<int> buffer(18);

    auto handle1 = buffer.allocate(sixteen.size());
    auto handle2 = buffer.allocate(eight.size());
    auto handle3 = buffer.allocate(eight.size());

    buffer.setData(handle1, sixteen);
    buffer.setData(handle2, eight);
    buffer.setData(handle3, eight);

    // Check that the whole data is continuous
    EXPECT_TRUE(checkContinuousData(buffer, handle1, { sixteen, eight, eight }));

    // Release one of the 8-length buffers and replace the data
    buffer.deallocate(handle2);
    
    handle2 = buffer.allocate(eightFrom6.size());
    EXPECT_EQ(buffer.getNumUsedElements(handle2), 0);
    buffer.setData(handle2, eightFrom6);
    EXPECT_EQ(buffer.getNumUsedElements(handle2), eightFrom6.size());

    EXPECT_TRUE(checkContinuousData(buffer, handle1, { sixteen, eightFrom6, eight }));

    buffer.deallocate(handle1);
    buffer.deallocate(handle2);
    buffer.deallocate(handle3);
}

TEST(ContinuousBufferTest, BlockReuseTightlyFit)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto sixteen = std::vector<int>({ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 });
    auto four = std::vector<int>({ 10,11,12,13 });

    render::ContinuousBuffer<int> buffer(18); // Allocate a buffer of size 18

    auto handle1 = buffer.allocate(eight.size());
    auto handle2 = buffer.allocate(sixteen.size());

    EXPECT_EQ(buffer.getNumUsedElements(handle1), 0);
    EXPECT_EQ(buffer.getNumUsedElements(handle2), 0);

    buffer.setData(handle1, eight);
    buffer.setData(handle2, sixteen);

    EXPECT_EQ(buffer.getNumUsedElements(handle1), eight.size());
    EXPECT_EQ(buffer.getNumUsedElements(handle2), sixteen.size());

    EXPECT_TRUE(checkContinuousData(buffer, handle1, { eight, sixteen }));

    // Release the 8-length buffers and replace the data with 2x 4
    buffer.deallocate(handle1);

    auto handle3 = buffer.allocate(four.size()); // this will leave a gap of 4
    auto handle4 = buffer.allocate(eight.size()); // this one will not fit into the gap
    auto handle5 = buffer.allocate(four.size()); // this one will

    buffer.setData(handle3, four);
    buffer.setData(handle4, eight);
    buffer.setData(handle5, four);

    EXPECT_TRUE(checkContinuousData(buffer, handle1, { four, four, sixteen, eight }));

    buffer.deallocate(handle2);
    buffer.deallocate(handle3);
    buffer.deallocate(handle4);
    buffer.deallocate(handle5);
}

// Allocates a 10 byte slot when the buffer is tightly fit (no free slot at the end)
// There are small free slots available in the middle of the buffer (#5959)
TEST(ContinuousBufferTest, ExpandBufferWithFreeSlotInTheMiddle)
{
    auto four = std::vector<int>({ 10,11,12,13 });
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });

    render::ContinuousBuffer<int> buffer(36); // Allocate a buffer of size 36

    // Allocate 9 slots of size 4 to fill the buffer, no space is left
    std::vector<render::ContinuousBuffer<int>::Handle> handles;
    std::vector<int> expectedData;

    for (auto i = 0; i < 9; ++i)
    {
        handles.push_back(buffer.allocate(four.size()));
        buffer.setData(handles.back(), four);
        expectedData.insert(expectedData.end(), four.begin(), four.end());
    }

    // Check the data, we should have 9 times the same sequence in the buffer
    EXPECT_TRUE(checkContinuousData(buffer, handles.front(), { expectedData }));

    // Free one block in the middle of the buffer
    auto freedHandle = handles[3];
    auto offsetToFreeBlock = buffer.getOffset(freedHandle);
    EXPECT_EQ(offsetToFreeBlock, 3 * four.size()) << "Wrong offset, expected free slot at 3*4 elements";
    buffer.deallocate(freedHandle);

    // Allocate a block of size 8, it won't fit in the free slot of size 4
    auto handle = buffer.allocate(eight.size());

    // The newly allocated block should not be located at the offset of that small free block
    // as it is too small - we expect it to be located somewhere more to the right.
    EXPECT_NE(buffer.getOffset(handle), offsetToFreeBlock) << "The 8-sized block should not be at the offset of the 4-sized hole";

    // Load some data into the new block, in the case of #5959 this corrupted data of other slots
    buffer.setData(handle, eight);

    // Compare the buffer contents to what we expect it to look like now (the 8 ints should have been moved to the end)
    std::copy(eight.begin(), eight.end(), std::back_inserter(expectedData));
    EXPECT_TRUE(checkContinuousData(buffer, handles.front(), { expectedData }));
}

TEST(ContinuousBufferTest, ExpandBufferWithFreeSlotInTheEnd)
{
    auto four = std::vector<int>({ 10,11,12,13 });
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });

    render::ContinuousBuffer<int> buffer(38); // Allocate a buffer of size 38

    // Allocate 9 slots of size 4 to fill the buffer, a slot of 2 will remain free
    std::vector<render::ContinuousBuffer<int>::Handle> handles;
    std::vector<int> expectedData;

    for (auto i = 0; i < 9; ++i)
    {
        handles.push_back(buffer.allocate(four.size()));
        buffer.setData(handles.back(), four);
        expectedData.insert(expectedData.end(), four.begin(), four.end());
    }

    // Check the data, we should have 9 times the same sequence in the buffer
    EXPECT_TRUE(checkContinuousData(buffer, handles.front(), { expectedData }));

    // Free one block in the middle of the buffer
    buffer.deallocate(handles[3]);

    // Allocate a block of size 8, it won't fit in the free slot of size 4 and not the one at the end
    auto handle = buffer.allocate(eight.size());

    // The newly allocated block should be located at the offset of the last free block
    EXPECT_EQ(buffer.getOffset(handle), expectedData.size()) << "The 8-sized block should have been put at the rightmost free block";

    // Load some data into the new block
    buffer.setData(handle, eight);

    // Compare the buffer contents to what we expect it to look like now (the 8 ints should have been moved to the end)
    std::copy(eight.begin(), eight.end(), std::back_inserter(expectedData));
    EXPECT_TRUE(checkContinuousData(buffer, handles.front(), { expectedData }));
}

TEST(ContinuousBufferTest, GapMerging)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto four = std::vector<int>({ 10,11,12,13 });
    auto sixteen = std::vector<int>({ 20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35 });

    render::ContinuousBuffer<int> buffer(18); // Allocate a buffer of size 18

    auto handle1 = buffer.allocate(four.size());
    auto handle2 = buffer.allocate(four.size());
    auto handle3 = buffer.allocate(four.size());
    auto handle4 = buffer.allocate(four.size());

    buffer.setData(handle1, four);
    buffer.setData(handle2, four);
    buffer.setData(handle3, four);
    buffer.setData(handle4, four);

    EXPECT_TRUE(checkContinuousData(buffer, handle1, { four, four, four, four }));

    // Release two continuous buffers (left one first) and try to fit in a 8-length data chunk
    buffer.deallocate(handle1);
    buffer.deallocate(handle2);

    auto handle5 = buffer.allocate(eight.size()); // this will fit tightly into the 2x4 gap
    buffer.setData(handle5, eight);

    EXPECT_TRUE(checkContinuousData(buffer, handle5, { eight, four, four }));

    // Release two more buffers (right one first) and try to fit in a 8-length data chunk
    buffer.deallocate(handle4); // rightmost first
    buffer.deallocate(handle3);

    auto handle6 = buffer.allocate(eight.size()); // this will fit tightly into the 2x4 gap
    buffer.setData(handle6, eight);

    EXPECT_TRUE(checkContinuousData(buffer, handle1, { eight, eight }));

    buffer.deallocate(handle5);
    buffer.deallocate(handle6);

    auto handle7 = buffer.allocate(sixteen.size()); // entire buffer is clear now
    buffer.setData(handle7, sixteen);

    EXPECT_EQ(buffer.getOffset(handle7), 0) << "This block should be starting at offset 0";
    EXPECT_TRUE(checkContinuousData(buffer, handle7, { sixteen }));
}

// Buffer will be tightly filled with no space left, allocation must succeed in this case
TEST(ContinuousBufferTest, ExpandFullBuffer)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto four = std::vector<int>({ 10,11,12,13 });

    render::ContinuousBuffer<int> buffer(eight.size() + four.size()); // Allocate a buffer that matches exactly

    auto handle1 = buffer.allocate(eight.size());
    auto handle2 = buffer.allocate(four.size());

    buffer.setData(handle1, eight);
    buffer.setData(handle2, four);

    EXPECT_TRUE(checkContinuousData(buffer, handle1, { eight, four}));

    // Allocate another piece of memory
    auto handle3 = buffer.allocate(eight.size());
    buffer.setData(handle3, eight);

    EXPECT_TRUE(checkContinuousData(buffer, handle1, { eight, four, eight }));
}

TEST(ContinuousBufferTest, SyncToBufferObject)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto four = std::vector<int>({ 10,11,12,13 });

    render::ContinuousBuffer<int> buffer(eight.size()); // Allocate a buffer that matches exactly
    auto bufferObject = std::make_shared<TestBufferObject>();

    // Allocate and fill in the eight bytes
    auto handle1 = buffer.allocate(eight.size());
    buffer.setData(handle1, eight);

    // Sync, it should then contain the 8 numbers
    auto modifiedData = eight;
    buffer.syncModificationsToBufferObject(bufferObject);
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle1, *bufferObject, modifiedData)) << "Data sync unsuccessful";

    // Upload the buffer partially
    buffer.setSubData(handle1, 3, four);
    std::copy(four.begin(), four.end(), modifiedData.begin() + 3);

    // Sync the buffer, it should now reflect the partially modified array
    buffer.syncModificationsToBufferObject(bufferObject);
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle1, *bufferObject, modifiedData)) << "Data sync unsuccessful";

    // Resize the buffer and sync again
    auto handle2 = buffer.allocate(eight.size());
    buffer.setData(handle2, eight);

    buffer.syncModificationsToBufferObject(bufferObject);
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle1, *bufferObject, modifiedData)) << "Data sync unsuccessful";
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle2, *bufferObject, eight)) << "Data sync unsuccessful";
}

TEST(ContinuousBufferTest, SyncToBufferAfterSubDataUpdate)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto four = std::vector<int>({ 10,11,12,13 });

    render::ContinuousBuffer<int> buffer(eight.size());

    auto bufferObject = std::make_shared<TestBufferObject>();

    // Allocate and fill in the eight bytes
    auto handle1 = buffer.allocate(eight.size());
    auto handle2 = buffer.allocate(eight.size());
    buffer.setData(handle1, eight);
    buffer.setData(handle2, eight);
 
    buffer.syncModificationsToBufferObject(bufferObject);
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle1, *bufferObject, eight)) << "Data sync unsuccessful";
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle2, *bufferObject, eight)) << "Data sync unsuccessful";

    // Modify a portion of the second slot
    std::size_t modificationOffset = 3;
    buffer.setSubData(handle2, modificationOffset, four);

    // Sync it should modify a subset of the buffer object only
    buffer.syncModificationsToBufferObject(bufferObject);

    // Check the offsets used to update the buffer object
    EXPECT_EQ(bufferObject->lastUsedOffset, (buffer.getOffset(handle2) + modificationOffset) * sizeof(int))
        << "Sync offset should point at the modification offset";
    EXPECT_EQ(bufferObject->lastUsedByteCount, four.size() * sizeof(int)) << "Only 4 bytes should have been synced";

    // First slot should still contain the 8 bytes
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle1, *bufferObject, eight)) << "Data sync unsuccessful";

    // Second slot should contain the 8 bytes, with 4 bytes overwritten at offset <modificationOffset>
    std::vector<int> modifiedSecondSlot = eight;
    std::copy(four.begin(), four.end(), modifiedSecondSlot.begin() + 3);
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle2, *bufferObject, modifiedSecondSlot)) << "Data sync unsuccessful";
}

// Checks that the partial update after applying a transaction log is working
TEST(ContinuousBufferTest, SyncToBufferAfterApplyingTransaction)
{
    auto eight = std::vector<int>({ 0,1,2,3,4,5,6,7 });
    auto four = std::vector<int>({ 10,11,12,13 });

    render::ContinuousBuffer<int> buffer(eight.size());

    auto bufferObject = std::make_shared<TestBufferObject>();

    // Allocate and fill in the eight bytes
    auto handle1 = buffer.allocate(eight.size());
    auto handle2 = buffer.allocate(eight.size());
    buffer.setData(handle1, eight);
    buffer.setData(handle2, eight);

    buffer.syncModificationsToBufferObject(bufferObject);
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle1, *bufferObject, eight)) << "Data sync unsuccessful";
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle2, *bufferObject, eight)) << "Data sync unsuccessful";

    // Copy this to a second buffer and change some data there
    auto buffer2 = buffer;
    std::size_t modificationOffset = 3;
    buffer2.setSubData(handle2, modificationOffset, four);

    // Define the transaction
    std::vector<render::detail::BufferTransaction> transactionLog;
    transactionLog.emplace_back(render::detail::BufferTransaction{
        handle2, modificationOffset, four.size()
    });

    // Apply this transaction to the first buffer
    buffer.applyTransactions(transactionLog, buffer2, [&](render::IGeometryStore::Slot slot) { return slot; });

    // Sync it should modify a subset of the buffer object only
    buffer.syncModificationsToBufferObject(bufferObject);

    // Check the offsets used to update the buffer object
    EXPECT_EQ(bufferObject->lastUsedOffset, (buffer.getOffset(handle2) + modificationOffset) * sizeof(int))
    << "Sync offset should at the modification offset";
    EXPECT_EQ(bufferObject->lastUsedByteCount, four.size() * sizeof(int)) << "Sync amount should be 4 unsigned ints";

    // First slot should contain the 8 bytes
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle1, *bufferObject, eight)) << "Data sync unsuccessful";

    // Second slot should contain the 8 bytes, with 4 bytes overwritten at offset <modificationOffset>
    std::vector<int> modifiedSecondSlot = eight;
    std::copy(four.begin(), four.end(), modifiedSecondSlot.begin() + modificationOffset);
    EXPECT_TRUE(checkDataInBufferObject(buffer, handle2, *bufferObject, eight)) << "Data sync unsuccessful";
}

}
