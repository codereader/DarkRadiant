#include "gtest/gtest.h"

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

TEST(GeometryStore, SyncObjectAcquisition)
{
    render::GeometryStore store(NullSyncObjectProvider::Instance());

    store.onFrameStart();
    store.onFrameFinished();

    EXPECT_EQ(NullSyncObjectProvider::Instance().invocationCount, 1) <<
        "GeometryStore should have acquired one sync object";
}

}
