#pragma once

#include <string>
#include <list>
#include <map>
#include "inode.h"
#include "imap.h"
#include "itextstream.h"

namespace map
{

namespace algorithm
{

struct ComparisonResult
{
    using Ptr = std::shared_ptr<ComparisonResult>;

    // Represents a matching node pair
    struct Match
    {
        std::string fingerPrint;
        scene::INodePtr sourceNode;
        scene::INodePtr targetNode;
    };

    struct Mismatch
    {
        std::string fingerPrint;
        scene::INodePtr sourceNode;
        scene::INodePtr targetNode;
    };

    // The collection of entities with the same fingerprint value
    std::list<Match> equivalentEntities;

    // The collection of entities with differing fingerprint values
    std::list<Mismatch> differingEntities;
};

class SceneGraphComparer
{
private:
    scene::IMapRootNodePtr _source;
    scene::IMapRootNodePtr _target;

    ComparisonResult::Ptr _result;

    using Fingerprints = std::map<std::string, scene::INodePtr>;
    using FingerprintsByType = std::map<scene::INode::Type, Fingerprints>;

public:
    SceneGraphComparer(const scene::IMapRootNodePtr& source, const scene::IMapRootNodePtr& target) :
        _source(source),
        _target(target),
        _result(new ComparisonResult)
    {}

    void compare();

    const ComparisonResult::Ptr& getResult() const
    {
        return _result;
    }

private:
    Fingerprints collectEntityFingerprints(const scene::INodePtr& root);
};

}

}
