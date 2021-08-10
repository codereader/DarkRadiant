#pragma once

#include <memory>
#include <list>
#include "imap.h"
#include "iselectiongroup.h"
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
    IMapRootNodePtr _sourceRoot;
    IMapRootNodePtr _baseRoot;

public:
    // Represents a matching node pair
    struct Match
    {
        std::string fingerPrint;
        INodePtr sourceNode;
        INodePtr baseNode;
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

        // A KeyValueDiff is the same if the same key is changed to the same value the same way
        bool operator==(const KeyValueDifference& other) const
        {
            return other.type == type && other.key == key && other.value == value;
        }
    };

    struct PrimitiveDifference
    {
        std::string fingerprint;
        INodePtr node;

        enum class Type
        {
            PrimitiveAdded,   // child is present on the source entity, but not on the base
            PrimitiveRemoved, // child is present on the base entity, but not on the source
        };

        Type type;
    };

    struct EntityDifference
    {
        INodePtr sourceNode;
        INodePtr baseNode;
        std::string entityName;
        std::string sourceFingerprint;
        std::string baseFingerprint;

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

    struct GroupDifference
    {
        std::shared_ptr<IGroupSelectable> sourceNode;
        std::shared_ptr<IGroupSelectable> baseNode;

        enum class Type
        {
            MembershipCountMismatch,    // nodes are member of a differing number of groups
            GroupMemberMismatch,        // membership count matches, but the groups have different members
        };

        Type type;
    };

public:
    ComparisonResult(const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& baseRoot) :
        _sourceRoot(sourceRoot),
        _baseRoot(baseRoot)
    {}

    // The collection of entities with the same fingerprint value
    std::list<Match> equivalentEntities;

    // The collection of differing entities
    std::list<EntityDifference> differingEntities;

    const IMapRootNodePtr& getBaseRootNode() const
    {
        return _baseRoot;
    }

    const IMapRootNodePtr& getSourceRootNode() const
    {
        return _sourceRoot;
    }
};

}

}
