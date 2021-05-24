#pragma once

#include <memory>

namespace scene
{ 

namespace merge
{

enum class ActionType
{
    
};

// Represents a single step of a merge process, like adding a brush,
// removing an entity, setting a keyvalue, etc.
class MergeAction
{
private:
    ActionType _type;

protected:
    MergeAction(ActionType type) :
        _type(type)
    {}

public:
    using Ptr = std::shared_ptr<MergeAction>;

    ActionType getType() const
    {
        return _type;
    }

    // Applies this action, changing the nodes affecting by it
    // It's the caller's responsibility to set up any Undo operations.
    // Implementations are allowed to throw std::runtime_errors on failure.
    virtual bool apply() = 0;
};


}

}
