#pragma once

#include "inode.h"
#include "scene/Entity.h"
#include "scenelib.h"

namespace map
{

class NodeCounter :
    public scene::NodeVisitor
{
private:
    std::size_t _count;
public:
    NodeCounter() :
        _count(0)
    {}

    bool pre(const scene::INodePtr& node)
    {
        if (Node_isPrimitive(node) || Node_isEntity(node))
        {
            _count++;
        }

        return true;
    }

    std::size_t getCount() const
    {
        return _count;
    }
};

}
