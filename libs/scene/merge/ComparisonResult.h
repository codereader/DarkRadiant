#pragma once

#include <memory>
#include <list>
#include "imap.h"
#include "inode.h"

namespace scene
{

namespace merge
{

class ComparisonResult
{
public:
    using Ptr = std::shared_ptr<ComparisonResult>;

private:
    // This result instance will hold references to the root nodes of the compared graph
    // to ensure the graph stays alive as long as the result
    scene::IMapRootNodePtr _sourceRoot;
    scene::IMapRootNodePtr _baseRoot;

public:
    // Represents a matching node pair
    struct Match
    {
        std::string fingerPrint;
        scene::INodePtr sourceNode;
        scene::INodePtr baseNode;
    };

    struct KeyValueDifference
    {
        std::string key;
        std::string value;

        enum class Type
        {
            KeyValueAdded,   // key is present on the source entity, but not on the base
            KeyValueRemoved, // key is present on the base entity, but not on the source
            KeyValueChanged, // key present on both, but value is different
        };

        Type type;
    };

    struct PrimitiveDifference
    {
        std::string fingerprint;
        scene::INodePtr node;

        enum class Type
        {
            PrimitiveAdded,   // child is present on the source entity, but not on the base
            PrimitiveRemoved, // child is present on the base entity, but not on the source
        };

        Type type;
    };

    struct EntityDifference
    {
        std::string fingerprint;
        scene::INodePtr node;
        std::string entityName;

        enum class Type
        {
            EntityMissingInSource,
            EntityMissingInBase,
            EntityPresentButDifferent,
        };

        Type type;

        std::list<KeyValueDifference> differingKeyValues;

        std::list<PrimitiveDifference> differingChildren;
    };

public:
    ComparisonResult(const scene::IMapRootNodePtr& sourceRoot, const scene::IMapRootNodePtr& baseRoot) :
        _sourceRoot(sourceRoot),
        _baseRoot(baseRoot)
    {}

    // The collection of entities with the same fingerprint value
    std::list<Match> equivalentEntities;

    // The collection of differing entities
    std::list<EntityDifference> differingEntities;

    const scene::IMapRootNodePtr& getBaseRootNode() const
    {
        return _baseRoot;
    }
};

}

}
