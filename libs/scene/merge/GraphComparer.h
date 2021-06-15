#pragma once

#include <string>
#include <list>
#include <map>
#include <memory>

#include "inode.h"
#include "imap.h"
#include "itextstream.h"

#include "ComparisonResult.h"

namespace scene
{

namespace merge
{

/**
 * Static utility class to compare two scenes given by their root nodes.
 * The Source is considered to be the "newer" graph with the changes,
 * the Base resembles the "older" or unchanged graph the Source graph is based on.
 */
class GraphComparer
{
private:
    using Fingerprints = std::map<std::string, INodePtr>;

public:
    struct EntityMismatch
    {
        std::string fingerPrint;
        INodePtr node;
        std::string entityName;
    };

    using EntityMismatchByName = std::map<std::string, EntityMismatch>;

public:
    // Compares the two graphs and returns the result
    static ComparisonResult::Ptr Compare(const IMapRootNodePtr& source, const IMapRootNodePtr& base);

private:
    static void processDifferingEntities(ComparisonResult& result, const EntityMismatchByName& sourceMismatches, 
        const EntityMismatchByName& baseMismatches);

    static std::list<ComparisonResult::KeyValueDifference> compareKeyValues(
        const INodePtr& sourceNode, const INodePtr& baseNode);

    static std::list<ComparisonResult::PrimitiveDifference> compareChildNodes(
        const INodePtr& sourceNode, const INodePtr& baseNode);

    static void compareSelectionGroups(ComparisonResult& result);
    static void compareSelectionGroups(ComparisonResult& result, const INodePtr& sourceNode, const INodePtr& baseNode);
    static void compareSelectionGroupsOfPrimitives(ComparisonResult& result, const INodePtr& sourceNode, const INodePtr& baseNode);
    static std::string calculateGroupFingerprint(const selection::ISelectionGroupPtr& group);
};

}

}
