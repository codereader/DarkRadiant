#pragma once

#include "igeometrystore.h"

namespace test
{

class TestSyncObjectProvider final :
    public render::ISyncObjectProvider
{
public:
    std::size_t invocationCount = 0;

    render::ISyncObject::Ptr createSyncObject() override
    {
        ++invocationCount;
        return {};
    }

    static TestSyncObjectProvider& Instance()
    {
        static TestSyncObjectProvider _instance;
        return _instance;
    }
};

}
