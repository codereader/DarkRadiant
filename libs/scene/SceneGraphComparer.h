#pragma once

#include <string>
#include <list>
#include <map>
#include "inode.h"
#include "imap.h"
#include "itextstream.h"

namespace scene
{

struct ComparisonResult
{
    using Ptr = std::shared_ptr<ComparisonResult>;

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
            KeyValueAdded,   // key is present on the source entity, but not on the target
            KeyValueRemoved, // key is present on the target entity, but not on the source
            KeyValueChanged, // key present on both, but value is different
        };

        Type type;
    };

    struct EntityDifference
    {
        std::string fingerPrint;
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
    };

    // The collection of entities with the same fingerprint value
    std::list<Match> equivalentEntities;

    // The collection of differing entities
    std::list<EntityDifference> differingEntities;
};

class SceneGraphComparer
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
    SceneGraphComparer(const scene::IMapRootNodePtr& source, const scene::IMapRootNodePtr& base) :
        _source(source),
        _base(base),
        _result(new ComparisonResult)
    {}

    void compare();

    const ComparisonResult::Ptr& getResult() const
    {
        return _result;
    }

private:
    void processDifferingEntities(const EntityMismatchByName& sourceMismatches, const EntityMismatchByName& targetMismatches);

    Fingerprints collectEntityFingerprints(const scene::INodePtr& root);

    std::list<ComparisonResult::KeyValueDifference> compareKeyValues(
        const scene::INodePtr& sourceNode, const scene::INodePtr& targetNode);
};

}
