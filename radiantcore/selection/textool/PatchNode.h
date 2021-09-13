#pragma once

#include "ipatch.h"
#include "NodeBase.h"

namespace textool
{

class PatchNode :
    public NodeBase
{
private:
    IPatch& _patch;

public:
    PatchNode(IPatch& patch) :
        _patch(patch)
    {}
};

}
