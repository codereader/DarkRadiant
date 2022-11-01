#pragma once

#include <vector>
#include "iselectiontest.h"

namespace selection
{

class SelectionTesterBase :
    public ISceneSelectionTester
{
private:
    std::vector<ISelectable*> _selectables;

public:
    bool hasSelectables() const override;
    void foreachSelectable(const std::function<void(ISelectable*)>& functor) override;
    void testSelectScene(const VolumeTest& view, SelectionTest& test) override;

protected:
    void storeSelectable(ISelectable* selectable);
    void storeSelectablesInPool(Selector& selector, const std::function<bool(ISelectable*)>& predicate);
};

/**
 * Tests entities and worldspawn primitives in the scene
 */
class PrimitiveSelectionTester :
    public SelectionTesterBase
{
public:
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
    ComponentSelectionTester(SelectionSystem& selectionSystem);

    void testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
        const std::function<bool(ISelectable*)>& predicate) override;
};

}
