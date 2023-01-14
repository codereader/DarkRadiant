#pragma once

#include <cstddef>
#include "inode.h"
#include "iundo.h"
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

// Produces a predicate object to check if a node is a brush with a certain material
inline std::function<bool(const scene::INodePtr&)> brushHasMaterial(const std::string& material)
{
    return [material](const scene::INodePtr& node) { return Node_isBrush(node) && Node_getIBrush(node)->hasShader(material); };
}

inline std::function<bool(const scene::INodePtr&)> patchHasMaterial(const std::string& material)
{
    return [material](const scene::INodePtr& node) { return Node_isPatch(node) && Node_getIPatch(node)->getShader() == material; };
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
    const std::function<bool(const IEntityNodePtr&)>& predicate)
{
    IEntityNodePtr candidate;

    parent->foreachNode([&](const scene::INodePtr& node)
    {
        auto entity = std::dynamic_pointer_cast<IEntityNode>(node);

        if (entity && predicate(entity))
        {
            candidate = entity;
            return false;
        }

        return true;
    });

    return candidate;
}

inline scene::INodePtr findWorldspawn(const scene::INodePtr& root)
{
    return findFirstEntity(root, [&](const IEntityNodePtr& entity)
    {
        return entity->getEntity().isWorldspawn();
    });
}

inline scene::INodePtr getEntityByName(const scene::INodePtr& parent, const std::string& name)
{
    return findFirstEntity(parent, [&](const IEntityNodePtr& entity)
    {
        return entity->getEntity().getKeyValue("name") == name;
    });
}

// Modifies a key/value of the worldspawn entity (in an Undoable transaction)
inline void setWorldspawnKeyValue(const std::string& key, const std::string& value)
{
    auto entity = GlobalMapModule().findOrInsertWorldspawn();

    UndoableCommand cmd("modifyKeyValue");
    Node_getEntity(entity)->setKeyValue(key, value);
}

inline scene::INodePtr findChildModelNode(const scene::INodePtr& parent)
{
    scene::INodePtr candidate;

    parent->foreachNode([&](const scene::INodePtr& node)
    {
        auto model = Node_getModel(node);

        if (model)
        {
            candidate = node;
            return false;
        }

        return true;
    });

    return candidate;
}

inline model::ModelNodePtr findChildModel(const scene::INodePtr& parent)
{
    return Node_getModel(findChildModelNode(parent));
}

// Returns the number of children of the given parent node matching the given predicate
inline std::size_t getChildCount(const scene::INodePtr& parent, 
    const std::function<bool(const scene::INodePtr&)>& predicate)
{
    std::size_t count = 0;

    parent->foreachNode([&](const scene::INodePtr& node)
    {
        if (predicate(node))
        {
            ++count;
        }

        return true;
    });

    return count;
}

// Returns the number of children of the given parent node
inline std::size_t getChildCount(const scene::INodePtr& parent)
{
    return getChildCount(parent, [](const scene::INodePtr&) { return true; });
}


}
