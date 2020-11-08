#pragma once

#include <cstddef>
#include "inode.h"
#include "ientity.h"
#include "ibrush.h"
#include "ipatch.h"
#include "imodel.h"

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

// Finds the first matching child patch of the given parent node matching the given predicate
inline scene::INodePtr findFirstPatch(const scene::INodePtr& parent,
    const std::function<bool(const IPatchNodePtr&)>& predicate)
{
    scene::INodePtr candidate;

    parent->foreachNode([&](const scene::INodePtr& node)
    {
        auto patchNode = std::dynamic_pointer_cast<IPatchNode>(node);

        if (patchNode && predicate(patchNode))
        {
            candidate = node;
            return false;
        }

        return true;
    });

    return candidate;
}

// Finds the first matching child patch of the given parent node, with the given material
inline scene::INodePtr findFirstPatchWithMaterial(const scene::INodePtr& parent, const std::string& material)
{
    return findFirstPatch(parent, [&](const IPatchNodePtr& patch)
    {
        return patch->getPatch().getShader() == material;
    });
}

inline scene::INodePtr findFirstEntity(const scene::INodePtr& parent,
    const std::function<bool(IEntityNode&)>& predicate)
{
    IEntityNodePtr candidate;

    parent->foreachNode([&](const scene::INodePtr& node)
    {
        auto entity = std::dynamic_pointer_cast<IEntityNode>(node);

        if (entity && predicate(*entity))
        {
            candidate = entity;
            return false;
        }

        return true;
    });

    return candidate;
}

inline scene::INodePtr getEntityByName(const scene::INodePtr& parent, const std::string& name)
{
    return findFirstEntity(parent, [&](IEntityNode& entity)
    {
        return entity.getEntity().getKeyValue("name") == name;
    });
}

inline model::ModelNodePtr findChildModel(const scene::INodePtr& parent)
{
    model::ModelNodePtr candidate;

    parent->foreachNode([&](const scene::INodePtr& node)
    {
        auto model = Node_getModel(node);

        if (model)
        {
            candidate = model;
            return false;
        }

        return true;
    });

    return candidate;
}

}
