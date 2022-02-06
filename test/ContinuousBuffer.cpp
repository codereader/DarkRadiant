#include "gtest/gtest.h"

#include "render/ContinuousBuffer.h"

namespace test
{

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
    buffer.setData(handle, sixteen);

    EXPECT_EQ(buffer.getOffset(handle), 0) << "Data should be located at offset 0";
    EXPECT_TRUE(checkData(buffer, handle, sixteen));

    buffer.deallocate(handle);

    // Re-allocate
    handle = buffer.allocate(sixteen.size());
    buffer.setData(handle, sixteen);

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

    buffer.setData(handle, eightAlt);
    EXPECT_TRUE(checkData(buffer, handle, eightAlt));
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
    buffer.setData(handle2, eightFrom6);

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

    buffer.setData(handle1, eight);
    buffer.setData(handle2, sixteen);

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

}
