#pragma once

#include "inode.h"
#include "icomparablenode.h"
#include "ientity.h"

namespace scene
{

namespace merge
{

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
        if (member->getNodeType() == INode::Type::Entity)
        {
            // Group links between entities should use the entity's name to check for equivalence
            // even if the entity has changed key values or primitives, the link is intact when the name is equal
            return GetEntityName(member);
        }

        auto comparable = std::dynamic_pointer_cast<IComparableNode>(member);

        if (comparable)
        {
            return comparable->getFingerprint();
        }
    }
};

}

}