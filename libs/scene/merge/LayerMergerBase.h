#pragma once

#include <sstream>

namespace scene
{

namespace merge
{

class LayerMergerBase
{
protected:
    std::stringstream _log;

protected:
    LayerMergerBase()
    {}

    std::string getLogMessages() const
    {
        return _log.str();
    }

    // Maps member fingerprint to node ref
    using LayerMembers = std::map<std::string, scene::INodePtr>;

    static void ForeachNodeInLayer(const INodePtr& root, int layerId, const std::function<void(const INodePtr&)>& functor)
    {
        root->foreachNode([&](const INodePtr& node)
        {
            if (node->getNodeType() != INode::Type::Entity &&
                node->getNodeType() != INode::Type::Brush &&
                node->getNodeType() != INode::Type::Patch)
            {
                return true;
            }

            if (node->getLayers().count(layerId) > 0)
            {
                functor(node);
            }

            return true;
        });
    }

    static LayerMembers GetLayerMemberFingerprints(const INodePtr& root, int layerId)
    {
        LayerMembers members;

        ForeachNodeInLayer(root, layerId, [&](const INodePtr& member)
        {
            members.emplace(NodeUtils::GetLayerMemberFingerprint(member), member);
        });

        return members;
    }
};

}

}
