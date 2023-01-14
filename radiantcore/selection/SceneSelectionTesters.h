#pragma once

#include <vector>
#include "iselectiontest.h"

namespace selection
{

class SelectionTestWalker;

/**
 * Filter function used when traversing the scene,
 * allowing to globally filter out nodes from the set of candidates.
 */
using NodePredicate = std::function<bool(const scene::INodePtr&)>;

class SelectionTesterBase :
    public ISceneSelectionTester
{
private:
    std::vector<ISelectable*> _selectables;
    NodePredicate _nodePredicate;

protected:
    SelectionTesterBase(const NodePredicate& nodePredicate);

public:
    bool hasSelectables() const override;
    void foreachSelectable(const std::function<void(ISelectable*)>& functor) override;
    void testSelectScene(const VolumeTest& view, SelectionTest& test) override;

protected:
    // Test the given node using the given tester. The node will be forwarded to the
    // tester only if it passed the predicate passed to the constructor
    void testNode(const scene::INodePtr& node, SelectionTestWalker& tester);

    bool nodeIsEligible(const scene::INodePtr& node) const;

    void storeSelectablesInPool(Selector& selector, const std::function<bool(ISelectable*)>& predicate);
    void storeSelectable(ISelectable* selectable);
};

/**
 * Tests entities and worldspawn primitives in the scene
 */
class PrimitiveSelectionTester :
    public SelectionTesterBase
{
public:
    PrimitiveSelectionTester(const NodePredicate& nodePredicate);

    void testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
        const std::function<bool(ISelectable*)>& predicate) override;

private:
    bool higherEntitySelectionPriority() const;
};

/**
 * Tests just the entities in the scene, all other nodes are skipped
 */
class EntitySelectionTester :
    public SelectionTesterBase
{
public:
    EntitySelectionTester(const NodePredicate& nodePredicate);

    void testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
        const std::function<bool(ISelectable*)>& predicate) override;
};

/**
 * Tests child primitives of group nodes only, non-worldspawn
 */
class GroupChildPrimitiveSelectionTester :
    public SelectionTesterBase
{
public:
    GroupChildPrimitiveSelectionTester(const NodePredicate& nodePredicate);

    void testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
        const std::function<bool(ISelectable*)>& predicate) override;
};

/**
 * This tester is looking for merge action nodes only
 */
class MergeActionSelectionTester :
    public SelectionTesterBase
{
public:
    MergeActionSelectionTester(const NodePredicate& nodePredicate);

    void testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
        const std::function<bool(ISelectable*)>& predicate) override;
};

/**
 * Tests components of selected scene elements
 */
class ComponentSelectionTester :
    public SelectionTesterBase
{
private:
    SelectionSystem& _selectionSystem;

public:
    ComponentSelectionTester(SelectionSystem& selectionSystem, const NodePredicate& nodePredicate);

    void testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
        const std::function<bool(ISelectable*)>& predicate) override;
};

}
