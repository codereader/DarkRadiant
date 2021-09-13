#pragma once

#include "ibrush.h"
#include "NodeBase.h"

namespace textool
{

class FaceNode :
    public NodeBase
{
private:
    IFace& _face;

public:
    FaceNode(IFace& face) :
        _face(face)
    {}
};

}
