#pragma once

#include <string>
#include <list>
#include <map>
#include <memory>

#include "inode.h"
#include "imap.h"
#include "itextstream.h"

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
};

class GraphComparer
{
private:
    scene::IMapRootNodePtr _source;
    scene::IMapRootNodePtr _base;

    ComparisonResult::Ptr _result;

    using Fingerprints = std::map<std::string, scene::INodePtr>;

public:
    struct EntityMismatch
    {
        std::string fingerPrint;
        scene::INodePtr node;
        std::string entityName;
    };

    using EntityMismatchByName = std::map<std::string, EntityMismatch>;

public:
    GraphComparer(const scene::IMapRootNodePtr& source, const scene::IMapRootNodePtr& base) :
        _source(source),
        _base(base),
        _result(new ComparisonResult(_source, _base))
    {}

    void compare();

    const ComparisonResult::Ptr& getResult() const
    {
        return _result;
    }

private:
    void processDifferingEntities(const EntityMismatchByName& sourceMismatches, const EntityMismatchByName& baseMismatches);

    Fingerprints collectEntityFingerprints(const scene::INodePtr& root);
    Fingerprints collectPrimitiveFingerprints(const scene::INodePtr& parent);

    Fingerprints collectNodeFingerprints(const scene::INodePtr& parent,
        const std::function<bool(const scene::INodePtr& node)>& nodePredicate);

    std::list<ComparisonResult::KeyValueDifference> compareKeyValues(
        const scene::INodePtr& sourceNode, const scene::INodePtr& baseNode);

    std::list<ComparisonResult::PrimitiveDifference> compareChildNodes(
        const scene::INodePtr& sourceNode, const scene::INodePtr& baseNode);
};

}

}
