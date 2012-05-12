#pragma once

/// Miscellaneous utility classes
namespace util
{

/// RAII object which sets a bool to true for its lifetime
class ScopedBoolLock
{
    bool& _target;

public:

    /// Construct and set target to true
    ScopedBoolLock(bool& target) :
        _target(target)
    {
        _target = true;
    }

    /// Destroy and set target to false
    ~ScopedBoolLock()
    {
        _target = false;
    }
};

}
