#include "DragManipulator.h"

#include "selection/SelectionPool.h"
#include "selection/SelectionTest.h"
#include "selection/algorithm/Planes.h"
#include "selection/SingleItemSelector.h"
#include "selection/BestSelector.h"

#include "registry/registry.h"

namespace selection
{

const std::string RKEY_TRANSIENT_COMPONENT_SELECTION = "user/ui/transientComponentSelection";

DragManipulator::DragManipulator(ManipulationPivot& pivot) :
	_pivot(pivot),
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

void DragManipulator::testSelect(const render::View& view, const Matrix4& pivot2world)
{
	_resizeModeActive = false;

    SelectionPool selector;
    SelectionVolume test(view);

	switch (GlobalSelectionSystem().Mode())
	{
	case SelectionSystem::ePrimitive:
		testSelectPrimitiveMode(view, test, selector);
		break;
	case SelectionSystem::eGroupPart:
		testSelectGroupPartMode(view, test, selector);
		break;
	case SelectionSystem::eEntity:
		testSelectEntityMode(view, test, selector);
		break;
	case SelectionSystem::eComponent:
		testSelectComponentMode(view, test, selector);
		break;
	};

	for (SelectionPool::const_iterator i = selector.begin(); i != selector.end(); ++i)
	{
		i->second->setSelected(true);
	}
}

void DragManipulator::testSelectPrimitiveMode(const render::View& view, SelectionVolume& test, SelectionPool& selector)
{
	SingleItemSelector itemSelector;

	// First try to select entities (including func_* groups)
	EntitySelector selectionTester(itemSelector, test);
	GlobalSceneGraph().foreachVisibleNodeInVolume(view, selectionTester);

	if (itemSelector.hasValidSelectable())
	{
		// Found a selectable entity
		selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
		return;
	}
	
	// Find all worldspawn primitives
	PrimitiveSelector primitiveTester(itemSelector, test);
	GlobalSceneGraph().foreachVisibleNodeInVolume(view, primitiveTester);

	if (itemSelector.hasValidSelectable())
	{
		// Found a selectable primitive
		selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
		return;
	}

	// Entities and worldspawn primitives failed, so check for group children too
	// Find all group child primitives that are selectable
	GroupChildPrimitiveSelector childPrimitiveTester(itemSelector, test);
	GlobalSceneGraph().foreachVisibleNodeInVolume(view, childPrimitiveTester);

	if (itemSelector.hasValidSelectable())
	{
		// Found a selectable group child primitive
		selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
		return;
	}

	// all direct hits failed, check for drag-selectable faces
	_resizeModeActive = algorithm::testSelectPlanes(selector, test);
}

void DragManipulator::testSelectGroupPartMode(const render::View& view, SelectionVolume& test, SelectionPool& selector)
{
	// Find all primitives that are selectable
	SingleItemSelector itemSelector;

	GroupChildPrimitiveSelector childPrimitiveTester(itemSelector, test);
	GlobalSceneGraph().foreachVisibleNodeInVolume(view, childPrimitiveTester);

	if (itemSelector.hasValidSelectable())
	{
		// Found a selectable primitive
		selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
		return;
	}

	// Check for selectable faces
	_resizeModeActive = algorithm::testSelectPlanes(selector, test);
}

void DragManipulator::testSelectEntityMode(const render::View& view, SelectionVolume& test, SelectionPool& selector)
{
	// Create a boolean selection pool (can have exactly one selectable or none)
	SingleItemSelector itemSelector;

	// Find the visible entities
	EntitySelector selectionTester(itemSelector, test);
	GlobalSceneGraph().foreachVisibleNodeInVolume(view, selectionTester);

	// Check, if an entity could be found
	if (itemSelector.hasValidSelectable())
	{
		selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
	}
}

void DragManipulator::testSelectComponentMode(const render::View& view, SelectionVolume& test, SelectionPool& selector)
{
	BestSelector bestSelector;

	ComponentSelector selectionTester(bestSelector, test, GlobalSelectionSystem().ComponentMode());
	GlobalSelectionSystem().foreachSelected(selectionTester);

	bool transientComponentSelection = registry::getValue<bool>(RKEY_TRANSIENT_COMPONENT_SELECTION);

	for (ISelectable* selectable : bestSelector.getBestSelectables())
	{
		// greebo: For transient component selection, clicking an unselected
		// component will deselect all previously selected components beforehand
		if (transientComponentSelection && !selectable->isSelected())
		{
			GlobalSelectionSystem().setSelectedAllComponents(false);
		}

		selector.addSelectable(SelectionIntersection(0, 0), selectable);
		_dragSelectable.setSelected(true);
	}
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
