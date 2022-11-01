#include "SceneSelectionTesters.h"

#include "iscenegraph.h"
#include "SelectionTestWalkers.h"

namespace selection
{

void PrimitiveSelectionTester::testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector)
{
    AnySelector anyTester(selector, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, anyTester);
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
