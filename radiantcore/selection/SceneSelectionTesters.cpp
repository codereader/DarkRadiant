#include "SceneSelectionTesters.h"

#include "iscenegraph.h"
#include "SelectionTestWalkers.h"

namespace selection
{

void EntitySelectionTester::testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector)
{
    EntitySelector entityTester(selector, test);
    GlobalSceneGraph().foreachVisibleNodeInVolume(view, entityTester);
}

}
