#include "SceneSelectionTesters.h"

#include "iscenegraph.h"
#include "SelectionTestWalkers.h"
#include "selection/EntitiesFirstSelector.h"
#include "selection/SelectionPool.h"

namespace selection
{

SelectionTesterBase::SelectionTesterBase(const NodePredicate& nodePredicate) :
    _nodePredicate(nodePredicate)
{}

bool SelectionTesterBase::nodeIsEligible(const scene::INodePtr& node) const
{
    return _nodePredicate ? _nodePredicate(node) : true;
}

void SelectionTesterBase::testNode(const scene::INodePtr& node, SelectionTestWalker& tester)
{
    if (nodeIsEligible(node))
    {
        tester.testNode(node);
    }
}

bool SelectionTesterBase::hasSelectables() const
{
    return !_selectables.empty();
}

void SelectionTesterBase::foreachSelectable(const std::function<void(ISelectable*)>& functor)
{
    for (auto selectable : _selectables)
    {
        functor(selectable);
    }
}

void SelectionTesterBase::testSelectScene(const VolumeTest& view, SelectionTest& test)
{
    // Forward to the specialised overload using an empty predicate
    testSelectSceneWithFilter(view, test, [](ISelectable*) { return true; });
}

void SelectionTesterBase::storeSelectable(ISelectable* selectable)
{
    _selectables.push_back(selectable);
}

void SelectionTesterBase::storeSelectablesInPool(Selector& selector, 
    const std::function<bool(ISelectable*)>& predicate)
{
    selector.foreachSelectable([&](auto selectable)
    {
        if (predicate(selectable))
        {
            storeSelectable(selectable);
        }
    });
}

PrimitiveSelectionTester::PrimitiveSelectionTester(const NodePredicate& nodePredicate) :
    SelectionTesterBase(nodePredicate)
{}

void PrimitiveSelectionTester::testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
    const std::function<bool(ISelectable*)>& predicate)
{
    SelectionPool simplePool;
    EntitiesFirstSelector sortedPool;

    // Sort entities before primitives if required, by referencing the correct selector
    auto& targetPool = !view.fill() && higherEntitySelectionPriority() ?
        static_cast<Selector&>(sortedPool) : simplePool;

    AnySelector anyTester(targetPool, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, [&](const scene::INodePtr& node)
    {
        testNode(node, anyTester);
        return true;
    });

    storeSelectablesInPool(targetPool, predicate);
}

bool PrimitiveSelectionTester::higherEntitySelectionPriority() const
{
    return registry::getValue<bool>(RKEY_HIGHER_ENTITY_PRIORITY);
}

EntitySelectionTester::EntitySelectionTester(const NodePredicate& nodePredicate) :
    SelectionTesterBase(nodePredicate)
{}

void EntitySelectionTester::testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
    const std::function<bool(ISelectable*)>& predicate)
{
    SelectionPool selector;

    EntitySelector tester(selector, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, [&](const scene::INodePtr& node)
    {
        testNode(node, tester);
        return true;
    });

    storeSelectablesInPool(selector, predicate);
}

GroupChildPrimitiveSelectionTester::GroupChildPrimitiveSelectionTester(const NodePredicate& nodePredicate) :
    SelectionTesterBase(nodePredicate)
{}

void GroupChildPrimitiveSelectionTester::testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
    const std::function<bool(ISelectable*)>& predicate)
{
    SelectionPool selector;

    GroupChildPrimitiveSelector tester(selector, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, [&](const scene::INodePtr& node)
    {
        testNode(node, tester);
        return true;
    });

    storeSelectablesInPool(selector, predicate);
}

MergeActionSelectionTester::MergeActionSelectionTester(const NodePredicate& nodePredicate) :
    SelectionTesterBase(nodePredicate)
{}

void MergeActionSelectionTester::testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
    const std::function<bool(ISelectable*)>& predicate)
{
    SelectionPool selector;

    MergeActionSelector tester(selector, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, [&](const scene::INodePtr& node)
    {
        testNode(node, tester);
        return true;
    });

    storeSelectablesInPool(selector, predicate);
}

ComponentSelectionTester::ComponentSelectionTester(SelectionSystem& selectionSystem, const NodePredicate& nodePredicate) :
    SelectionTesterBase(nodePredicate),
    _selectionSystem(selectionSystem)
{}

void ComponentSelectionTester::testSelectSceneWithFilter(const VolumeTest& view, SelectionTest& test,
    const std::function<bool(ISelectable*)>& predicate)
{
    SelectionPool selector;

    ComponentSelector tester(selector, test, _selectionSystem.ComponentMode());
    _selectionSystem.foreachSelected([&](const scene::INodePtr& node)
    {
        testNode(node, tester);
    });

    storeSelectablesInPool(selector, predicate);
}

}
