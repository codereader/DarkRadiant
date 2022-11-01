#include "SceneSelectionTesters.h"

#include "iscenegraph.h"
#include "registry/registry.h"
#include "SelectionTestWalkers.h"
#include "selection/SelectionPool.h"

namespace selection
{

void PrimitiveSelectionTester::testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector)
{
    // Cameras don't sort entities higher
    if (view.fill() || !higherEntitySelectionPriority())
    {
        // Test for any visible elements (primitives, entities), but don't select child primitives
        AnySelector anyTester(selector, test);
        GlobalSceneGraph().foreachVisibleNodeInVolume(view, anyTester);
        return;
    }

    // We have an orthoview, select entities first
    SelectionPool entityPool;
    SelectionPool primitivePool;

    // First, obtain all the selectable entities
    EntitySelector entityTester(entityPool, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, entityTester);

    // Now retrieve all the selectable primitives
    PrimitiveSelector primitiveTester(primitivePool, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, primitiveTester);

    // Transfer the items of both pools to the target selector, entities first
    for (const auto& [intersection, selectable] : entityPool)
    {
        selector.pushSelectable(*selectable);
        selector.addIntersection(intersection);
        selector.popSelectable();
    }

    for (const auto& [intersection, selectable] : primitivePool)
    {
        // Check for duplicates
        SelectionPool::const_iterator existing;

        for (existing = entityPool.begin(); existing != entityPool.end(); ++existing)
        {
            if (existing->second == selectable) break;
        }

        if (existing == entityPool.end())
        {
            selector.pushSelectable(*selectable);
            selector.addIntersection(intersection);
            selector.popSelectable();
        }
    }
}

bool PrimitiveSelectionTester::higherEntitySelectionPriority() const
{
    return registry::getValue<bool>(RKEY_HIGHER_ENTITY_PRIORITY);
}

void EntitySelectionTester::testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector)
{
    EntitySelector tester(selector, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, tester);
}

void GroupChildPrimitiveSelectionTester::testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector)
{
    GroupChildPrimitiveSelector tester(selector, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, tester);
}

void MergeActionSelectionTester::testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector)
{
    MergeActionSelector tester(selector, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, tester);
}

ComponentSelectionTester::ComponentSelectionTester(SelectionSystem& selectionSystem) :
    _selectionSystem(selectionSystem)
{}

void ComponentSelectionTester::testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector)
{
    ComponentSelector selectionTester(selector, test, _selectionSystem.ComponentMode());
    _selectionSystem.foreachSelected(selectionTester);
}

}
