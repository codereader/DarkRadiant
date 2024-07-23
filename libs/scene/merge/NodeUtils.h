#pragma once

#include <map>
#include "inode.h"
#include "icomparablenode.h"
#include "ientity.h"
#include "itextstream.h"
#include "scene/Entity.h"

namespace scene
{

namespace merge
{

using Fingerprints = std::map<std::string, INodePtr>;

class NodeUtils
{
public:
    static std::string GetEntityName(const INodePtr& node)
    {
        assert(node->getNodeType() == INode::Type::Entity);
        auto entity = Node_getEntity(node);

        return entity->isWorldspawn() ? "worldspawn" : entity->getKeyValue("name");
    }

    static std::string GetGroupMemberFingerprint(const INodePtr& member)
    {
        return GetEntityNameOrFingerprint(member);
    }

    static std::string GetLayerMemberFingerprint(const INodePtr& member)
    {
        return GetEntityNameOrFingerprint(member);
    }

    static Fingerprints CollectEntityFingerprints(const INodePtr& root)
    {
        return CollectNodeFingerprints(root, [](const INodePtr& node)
        {
            return node->getNodeType() == INode::Type::Entity;
        });
    }

    static Fingerprints CollectPrimitiveFingerprints(const INodePtr& parent)
    {
        return CollectNodeFingerprints(parent, [](const INodePtr& node)
        {
            return node->getNodeType() == INode::Type::Brush || node->getNodeType() == INode::Type::Patch;
        });
    }

private:
    static Fingerprints CollectNodeFingerprints(const INodePtr& parent,
        const std::function<bool(const INodePtr& node)>& nodePredicate)
    {
        Fingerprints result;

        parent->foreachNode([&](const INodePtr& node)
        {
            if (!nodePredicate(node)) return true; // predicate says "skip"

            auto comparable = std::dynamic_pointer_cast<IComparableNode>(node);
            assert(comparable);

            if (!comparable) return true; // skip

            // Store the fingerprint and check for collisions
            auto insertResult = result.try_emplace(comparable->getFingerprint(), node);

            if (!insertResult.second)
            {
                rWarning() << "More than one node with the same fingerprint found in the parent node with name " << parent->name() << std::endl;
            }

            return true;
        });

        return result;
    }

    static std::string GetEntityNameOrFingerprint(const INodePtr& member)
    {
        if (member->getNodeType() == INode::Type::Entity)
        {
            // Links between entities should use the entity's name to check for equivalence
            // even if the entity has changed key values or primitives, the link is intact when the name is equal
            return GetEntityName(member);
        }

        auto comparable = std::dynamic_pointer_cast<IComparableNode>(member);

        if (comparable)
        {
            return comparable->getFingerprint();
        }

        return std::string();
    }
};

}

}
