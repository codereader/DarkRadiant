#include "DragManipulator.h"

#include "selection/SelectionPool.h"
#include "selection/SelectionTestWalkers.h"
#include "selection/algorithm/Planes.h"

namespace selection
{

namespace
{
    // Predicate function used to pick selectable Sectables only when drag-manipulating
    bool filterSelectedItemsOnly(ISelectable* selectable)
    {
        return selectable->isSelected();
    }
}

DragManipulator::DragManipulator(ManipulationPivot& pivot, SelectionSystem& selectionSystem, ISceneSelectionTesterFactory& factory) :
    _pivot(pivot),
    _selectionSystem(selectionSystem),
    _testerFactory(factory),
    _freeResizeComponent(_resizeTranslatable),
    _resizeModeActive(false),
    _freeDragComponent(_dragTranslatable),
    _dragTranslatable(SelectionTranslator::TranslationCallback())
{}

DragManipulator::Type DragManipulator::getType() const
{
	return Drag;
}

DragManipulator::Component* DragManipulator::getActiveComponent() 
{
    return _dragSelectable.isSelected() ? &_freeDragComponent : &_freeResizeComponent;
}

void DragManipulator::testSelect(SelectionTest& test, const Matrix4& pivot2world)
{
	_resizeModeActive = false;

    // No drag manipulation in merge mode
    if (_selectionSystem.getSelectionMode() == SelectionMode::MergeAction) return;

    SelectionPool selector;

	switch (_selectionSystem.getSelectionMode())
	{
	case SelectionMode::Primitive:
		testSelectPrimitiveMode(test.getVolume(), test, selector);
		break;
	case SelectionMode::GroupPart:
		testSelectGroupPartMode(test.getVolume(), test, selector);
		break;
	case SelectionMode::Entity:
		testSelectEntityMode(test.getVolume(), test, selector);
		break;
	case SelectionMode::Component:
		testSelectComponentMode(test.getVolume(), test, selector);
		break;
	default:
        return;
	};

	for (auto& [_, selectable] : selector)
	{
		selectable->setSelected(true);
	}
}

bool DragManipulator::testSelectedItemsInScene(SelectionMode mode, const VolumeTest& view, SelectionTest& test)
{
    auto tester = _testerFactory.createSceneSelectionTester(mode);
    tester->testSelectSceneWithFilter(view, test, filterSelectedItemsOnly);

    return tester->hasSelectables();
}

void DragManipulator::testSelectPrimitiveMode(const VolumeTest& view, SelectionTest& test, SelectionPool& selector)
{
    // If testing for entities and worldspawn primitives fails check for group children too
    if (testSelectedItemsInScene(SelectionMode::Primitive, view, test) ||
        testSelectedItemsInScene(SelectionMode::GroupPart, view, test))
    {
        selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
        return;
    }

    // all direct hits failed, check for drag-selectable faces
    _resizeModeActive = algorithm::testSelectPlanes(selector, test);
}

void DragManipulator::testSelectGroupPartMode(const VolumeTest& view, SelectionTest& test, SelectionPool& selector)
{
	// Find all non-worldspawn child primitives that are selectable
	if (testSelectedItemsInScene(SelectionMode::GroupPart, view, test))
	{
		// Found a selectable primitive
		selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
		return;
	}

	// Check for selectable faces
	_resizeModeActive = algorithm::testSelectPlanes(selector, test);
}

void DragManipulator::testSelectEntityMode(const VolumeTest& view, SelectionTest& test, SelectionPool& selector)
{
	// Check, if an entity could be found
	if (testSelectedItemsInScene(SelectionMode::Entity, view, test))
	{
		selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
        return;
	}

    // Check for selectable faces
    _resizeModeActive = algorithm::testSelectPlanes(selector, test);
}

void DragManipulator::testSelectComponentMode(const VolumeTest& view, SelectionTest& test, SelectionPool& selector)
{
    auto tester = _testerFactory.createSceneSelectionTester(SelectionMode::Component);
    tester->testSelectScene(view, test); // don't restrict drag-selecting to selected components

    tester->foreachSelectable([&](auto selectable)
    {
        // greebo: Transient component selection: clicking an unselected
        // component will deselect all previously selected components beforehand
        if (!selectable->isSelected())
        {
            _selectionSystem.setSelectedAllComponents(false);
        }

        selector.addSelectable(SelectionIntersection(0, 0), selectable);
        _dragSelectable.setSelected(true);
    });
}

void DragManipulator::setSelected(bool select) 
{
    _resizeModeActive = select;
    _dragSelectable.setSelected(select);
}

bool DragManipulator::isSelected() const
{
	return _resizeModeActive || _dragSelectable.isSelected();
}

}
