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

    }
};

}

}
