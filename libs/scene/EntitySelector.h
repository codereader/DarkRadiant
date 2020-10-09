#pragma once

#include "inode.h"
#include "ientity.h"
#include "iselectable.h"

namespace scene
{

/**
 * Scene walker (de-)selecting all entities
 * matching the predicate passed to the constructor.
 */
class EntitySelector :
    public scene::NodeVisitor
{
private:
    std::function<bool(const Entity&)> _predicate;
    bool _select;

public:
    EntitySelector(const std::function<bool(const Entity&)>& predicate, bool select) :
        _predicate(predicate),
        _select(select)
    {}

    bool pre(const scene::INodePtr& node) override
    {
        if (!Node_isEntity(node))
        {
            return true;
        }

        const auto* entity = Node_getEntity(node);
        assert(entity != nullptr);

        if (_predicate(*entity))
        {
            Node_setSelected(node, _select);
        }

        return false; // don't go deeper
    }
};

}
