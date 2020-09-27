#pragma once

#include <cstddef>
#include "inode.h"
#include "ientity.h"

namespace test::algorithm
{

// Returns the n-th child of the given parent node
inline scene::INodePtr getNthChild(const scene::INodePtr& parent, std::size_t index)
{
    std::size_t n = 0;
    scene::INodePtr candidate;

    parent->foreachNode([&](const scene::INodePtr& node)
    {
        if (n == index)
        {
            candidate = node;
            return false;
        }

        ++n;
        return true;
    });

    return candidate;
}

}
