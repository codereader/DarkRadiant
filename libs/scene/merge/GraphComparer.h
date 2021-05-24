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
