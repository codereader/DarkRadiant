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
    auto fullData = sixteen;
    fullData.insert(fullData.end(), eight.begin(), eight.end());
    fullData.insert(fullData.end(), eight.begin(), eight.end());

    EXPECT_TRUE(checkData(buffer, handle1, fullData)) << "Data not continuously stored";

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
    auto fullData = sixteen;
    fullData.insert(fullData.end(), eight.begin(), eight.end());
    fullData.insert(fullData.end(), eight.begin(), eight.end());

    EXPECT_TRUE(checkData(buffer, handle1, fullData)) << "Data not continuously stored";

    // Release one of the 8-length buffers and replace the data
    buffer.deallocate(handle2);
    handle2 = buffer.allocate(eightFrom6.size());

    buffer.setData(handle2, eightFrom6);

    fullData = sixteen;
    fullData.insert(fullData.end(), eightFrom6.begin(), eightFrom6.end());
    fullData.insert(fullData.end(), eight.begin(), eight.end());

    EXPECT_TRUE(checkData(buffer, handle1, fullData)) << "New data not continuously stored";

    buffer.deallocate(handle1);
    buffer.deallocate(handle2);
    buffer.deallocate(handle3);
}

}
