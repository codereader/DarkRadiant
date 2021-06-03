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

    static Fingerprints collectEntityFingerprints(const INodePtr& root);
    static Fingerprints collectPrimitiveFingerprints(const INodePtr& parent);

    static Fingerprints collectNodeFingerprints(const INodePtr& parent,
        const std::function<bool(const INodePtr& node)>& nodePredicate);

    static std::list<ComparisonResult::KeyValueDifference> compareKeyValues(
        const INodePtr& sourceNode, const INodePtr& baseNode);

    static std::list<ComparisonResult::PrimitiveDifference> compareChildNodes(
        const INodePtr& sourceNode, const INodePtr& baseNode);

    static void compareSelectionGroups(ComparisonResult& result);
    static void compareSelectionGroups(ComparisonResult& result, const INodePtr& sourceNode, const INodePtr& baseNode);
    static std::string calculateGroupFingerprint(const selection::ISelectionGroupPtr& group);
};

}

}
