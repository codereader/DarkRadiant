#pragma once

#include "SelectionGroupMergerBase.h"

namespace scene
{

namespace merge
{

class ThreeWaySelectionGroupMerger :
    public SelectionGroupMergerBase
{
private:
    IMapRootNodePtr _baseRoot;
    IMapRootNodePtr _sourceRoot;
    IMapRootNodePtr _targetRoot;

    selection::ISelectionGroupManager& _baseManager;
    selection::ISelectionGroupManager& _sourceManager;
    selection::ISelectionGroupManager& _targetManager;

    NodeFingerprints _baseNodes;
    NodeFingerprints _sourceNodes;
    NodeFingerprints _targetNodes;

public:
    ThreeWaySelectionGroupMerger(const IMapRootNodePtr& baseRoot, const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& targetRoot) :
        _baseRoot(baseRoot),
        _sourceRoot(sourceRoot),
        _targetRoot(targetRoot),
        _baseManager(_baseRoot->getSelectionGroupManager()),
        _sourceManager(_sourceRoot->getSelectionGroupManager()),
        _targetManager(_targetRoot->getSelectionGroupManager())
    {}

    const IMapRootNodePtr& getSourceRoot() const
    {
        return _sourceRoot;
    }

    const IMapRootNodePtr& getTargetRoot() const
    {
        return _targetRoot;
    }

    const IMapRootNodePtr& getBaseRoot() const
    {
        return _baseRoot;
    }
    
    void adjustTargetGroups()
    {
        // Collect all node fingerprints for easier lookup
        _sourceNodes = collectNodeFingerprints(_sourceRoot);
        _log << "Got " << _sourceNodes.size() << " groups in the source map" << std::endl;

        _targetNodes = collectNodeFingerprints(_targetRoot);
        _log << "Got " << _targetNodes.size() << " in the target map" << std::endl;

        _baseNodes = collectNodeFingerprints(_baseRoot);
        _log << "Got " << _baseNodes.size() << " in the base map" << std::endl;
    }
};

}

}
