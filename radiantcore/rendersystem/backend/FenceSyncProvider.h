#pragma once

#include <stdexcept>
#include "igl.h"
#include "igeometrystore.h"

namespace render
{

// ISyncObjectProvider implementation based on glFenceSync/glClientWaitSync
class FenceSyncProvider final :
    public ISyncObjectProvider
{
    class FenceSync final :
        public ISyncObject
    {
    private:
        GLsync _syncObject;

    public:
        FenceSync() :
            _syncObject(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0))
        {}

        void wait() override
        {
            if (_syncObject == nullptr) return;

            auto result = glClientWaitSync(_syncObject, 0, GL_TIMEOUT_IGNORED);

            while (result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED)
            {
                result = glClientWaitSync(_syncObject, 0, GL_TIMEOUT_IGNORED);

                if (result == GL_WAIT_FAILED)
                {
                    throw std::runtime_error("Could not acquire frame buffer lock");
                }
            }
        }

        ~FenceSync()
        {
            glDeleteSync(_syncObject);
            _syncObject = nullptr;
        }
    };

public:
    ISyncObject::Ptr createSyncObject() override
    {
        return std::make_shared<FenceSync>();
    }
};

}
