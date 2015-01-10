#pragma once

#include "inode.h"
#include <functional>

class IMapFileChangeTracker
{
public:
    virtual ~IMapFileChangeTracker() {}

    virtual void save() = 0;
    virtual bool saved() const = 0;
    virtual void changed() = 0;
    virtual void setChangedCallback(const std::function<void()>& changed) = 0;
    virtual std::size_t changes() const = 0;
};
