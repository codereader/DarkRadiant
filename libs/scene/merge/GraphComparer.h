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
    // Compares the two graphs and returns the result
    static ComparisonResult::Ptr Compare(const scene::IMapRootNodePtr& source, const scene::IMapRootNodePtr& base);

private:
    static void processDifferingEntities(ComparisonResult& result, const EntityMismatchByName& sourceMismatches, 
        const EntityMismatchByName& baseMismatches);

    static Fingerprints collectEntityFingerprints(const scene::INodePtr& root);
    static Fingerprints collectPrimitiveFingerprints(const scene::INodePtr& parent);

    static Fingerprints collectNodeFingerprints(const scene::INodePtr& parent,
        const std::function<bool(const scene::INodePtr& node)>& nodePredicate);

    static std::list<ComparisonResult::KeyValueDifference> compareKeyValues(
        const scene::INodePtr& sourceNode, const scene::INodePtr& baseNode);

    static std::list<ComparisonResult::PrimitiveDifference> compareChildNodes(
        const scene::INodePtr& sourceNode, const scene::INodePtr& baseNode);
};

}

}
