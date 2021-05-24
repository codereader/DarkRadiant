#pragma once

namespace scene
{ 

namespace merge
{

enum class ActionType
{
    
};

// Represents a single operation during a merge process
// There are various types of actions, i.e. brush addition,
// entity removal, keyvalue change, etc.
class MergeAction
{
private:
    ActionType _type;

public:
    MergeAction(ActionType type) :
        _type(type)
    {}

    ActionType getType() const
    {
        return _type;
    }
};


}

}
