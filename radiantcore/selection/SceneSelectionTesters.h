#pragma once

#include "iselectiontest.h"

namespace selection
{
/**
 * Tests just the entities in the scene, all other nodes are skipped
 */
class EntitySelectionTester :
    public ISceneSelectionTester
{
public:
    void testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector) override;
};
    
}
