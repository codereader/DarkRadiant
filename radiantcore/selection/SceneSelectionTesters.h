#pragma once

#include "iselectiontest.h"

namespace selection
{

/**
 * Tests entities and primitives in the scene, entities sorted first
 */
class PrimitiveSelectionTester :
    public ISceneSelectionTester
{
public:
    void testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector) override;

private:
    bool higherEntitySelectionPriority() const;
};

/**
 * Tests just the entities in the scene, all other nodes are skipped
 */
class EntitySelectionTester :
    public ISceneSelectionTester
{
public:
    void testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector) override;
};

/**
 * Tests child primitives of group nodes only, non-worldspawn
 */
class GroupChildPrimitiveSelectionTester :
    public ISceneSelectionTester
{
public:
    void testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector) override;
};

/**
 * This tester is looking for merge action nodes only
 */
class MergeActionSelectionTester :
    public ISceneSelectionTester
{
public:
    void testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector) override;
};

/**
 * Tests components of selected scene elements
 */
class ComponentSelectionTester :
    public ISceneSelectionTester
{
private:
    SelectionSystem& _selectionSystem;

public:
    ComponentSelectionTester(SelectionSystem& selectionSystem);

    void testSelectScene(const VolumeTest& view, SelectionTest& test, Selector& selector) override;
};

}
