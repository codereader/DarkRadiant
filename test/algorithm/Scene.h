#pragma once

#include <cstddef>
#include "inode.h"
#include "ientity.h"
#include "ibrush.h"

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

// Finds the first matching child brush of the given parent node matching the given predicate
inline scene::INodePtr findFirstBrush(const scene::INodePtr& parent,
    const std::function<bool(const IBrushNodePtr&)>& predicate)
{
    scene::INodePtr candidate;

    parent->foreachNode([&](const scene::INodePtr& node)
    {
        auto brushNode = std::dynamic_pointer_cast<IBrushNode>(node);

        if (brushNode && predicate(brushNode))
        {
            candidate = node;
            return false;
        }

        return true;
    });

    return candidate;
}

// Finds the first matching child brush of the given parent node, with any of the brush's faces matching the given material
inline scene::INodePtr findFirstBrushWithMaterial(const scene::INodePtr& parent, const std::string& material)
{
    return findFirstBrush(parent, [&](const IBrushNodePtr& brush)
    {
        return brush->getIBrush().hasShader(material);
    });
}

}
